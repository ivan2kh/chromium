// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/connector.h"

#include <stdint.h>
#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "mojo/public/cpp/bindings/lib/may_auto_lock.h"
#include "mojo/public/cpp/bindings/sync_handle_watcher.h"
#include "mojo/public/cpp/system/wait.h"

namespace mojo {

Connector::Connector(ScopedMessagePipeHandle message_pipe,
                     ConnectorConfig config,
                     scoped_refptr<base::SingleThreadTaskRunner> runner)
    : message_pipe_(std::move(message_pipe)),
      task_runner_(std::move(runner)),
      weak_factory_(this) {
  if (config == MULTI_THREADED_SEND)
    lock_.emplace();

  weak_self_ = weak_factory_.GetWeakPtr();
  // Even though we don't have an incoming receiver, we still want to monitor
  // the message pipe to know if is closed or encounters an error.
  WaitToReadMore();
}

Connector::~Connector() {
  {
    // Allow for quick destruction on any thread if the pipe is already closed.
    base::AutoLock lock(connected_lock_);
    if (!connected_)
      return;
  }

  DCHECK(thread_checker_.CalledOnValidThread());
  CancelWait();
}

void Connector::CloseMessagePipe() {
  // Throw away the returned message pipe.
  PassMessagePipe();
}

ScopedMessagePipeHandle Connector::PassMessagePipe() {
  DCHECK(thread_checker_.CalledOnValidThread());

  CancelWait();
  internal::MayAutoLock locker(&lock_);
  ScopedMessagePipeHandle message_pipe = std::move(message_pipe_);
  weak_factory_.InvalidateWeakPtrs();
  sync_handle_watcher_callback_count_ = 0;

  base::AutoLock lock(connected_lock_);
  connected_ = false;
  return message_pipe;
}

void Connector::RaiseError() {
  DCHECK(thread_checker_.CalledOnValidThread());

  HandleError(true, true);
}

bool Connector::WaitForIncomingMessage(MojoDeadline deadline) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (error_)
    return false;

  ResumeIncomingMethodCallProcessing();

  // TODO(rockot): Use a timed Wait here. Nobody uses anything but 0 or
  // INDEFINITE deadlines at present, so we only support those.
  DCHECK(deadline == 0 || deadline == MOJO_DEADLINE_INDEFINITE);

  MojoResult rv = MOJO_RESULT_UNKNOWN;
  if (deadline == 0 && !message_pipe_->QuerySignalsState().readable())
    return false;

  if (deadline == MOJO_DEADLINE_INDEFINITE) {
    rv = Wait(message_pipe_.get(), MOJO_HANDLE_SIGNAL_READABLE);
    if (rv != MOJO_RESULT_OK) {
      // Users that call WaitForIncomingMessage() should expect their code to be
      // re-entered, so we call the error handler synchronously.
      HandleError(rv != MOJO_RESULT_FAILED_PRECONDITION, false);
      return false;
    }
  }

  ignore_result(ReadSingleMessage(&rv));
  return (rv == MOJO_RESULT_OK);
}

void Connector::PauseIncomingMethodCallProcessing() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (paused_)
    return;

  paused_ = true;
  CancelWait();
}

void Connector::ResumeIncomingMethodCallProcessing() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!paused_)
    return;

  paused_ = false;
  WaitToReadMore();
}

bool Connector::Accept(Message* message) {
  DCHECK(lock_ || thread_checker_.CalledOnValidThread());

  // It shouldn't hurt even if |error_| may be changed by a different thread at
  // the same time. The outcome is that we may write into |message_pipe_| after
  // encountering an error, which should be fine.
  if (error_)
    return false;

  internal::MayAutoLock locker(&lock_);

  if (!message_pipe_.is_valid() || drop_writes_)
    return true;

  MojoResult rv =
      WriteMessageNew(message_pipe_.get(), message->TakeMojoMessage(),
                      MOJO_WRITE_MESSAGE_FLAG_NONE);

  switch (rv) {
    case MOJO_RESULT_OK:
      break;
    case MOJO_RESULT_FAILED_PRECONDITION:
      // There's no point in continuing to write to this pipe since the other
      // end is gone. Avoid writing any future messages. Hide write failures
      // from the caller since we'd like them to continue consuming any backlog
      // of incoming messages before regarding the message pipe as closed.
      drop_writes_ = true;
      break;
    case MOJO_RESULT_BUSY:
      // We'd get a "busy" result if one of the message's handles is:
      //   - |message_pipe_|'s own handle;
      //   - simultaneously being used on another thread; or
      //   - in a "busy" state that prohibits it from being transferred (e.g.,
      //     a data pipe handle in the middle of a two-phase read/write,
      //     regardless of which thread that two-phase read/write is happening
      //     on).
      // TODO(vtl): I wonder if this should be a |DCHECK()|. (But, until
      // crbug.com/389666, etc. are resolved, this will make tests fail quickly
      // rather than hanging.)
      CHECK(false) << "Race condition or other bug detected";
      return false;
    default:
      // This particular write was rejected, presumably because of bad input.
      // The pipe is not necessarily in a bad state.
      return false;
  }
  return true;
}

void Connector::AllowWokenUpBySyncWatchOnSameThread() {
  DCHECK(thread_checker_.CalledOnValidThread());

  allow_woken_up_by_others_ = true;

  EnsureSyncWatcherExists();
  sync_watcher_->AllowWokenUpBySyncWatchOnSameThread();
}

bool Connector::SyncWatch(const bool* should_stop) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (error_)
    return false;

  ResumeIncomingMethodCallProcessing();

  EnsureSyncWatcherExists();
  return sync_watcher_->SyncWatch(should_stop);
}

void Connector::SetWatcherHeapProfilerTag(const char* tag) {
  heap_profiler_tag_ = tag;
  if (handle_watcher_) {
    handle_watcher_->set_heap_profiler_tag(tag);
  }
}

void Connector::EnableNestedDispatch(bool enabled) {
  nested_dispatch_enabled_ = enabled;
  handle_watcher_.reset();
  WaitToReadMore();
}

void Connector::OnWatcherHandleReady(MojoResult result) {
  OnHandleReadyInternal(result);
}

void Connector::OnSyncHandleWatcherHandleReady(MojoResult result) {
  base::WeakPtr<Connector> weak_self(weak_self_);

  sync_handle_watcher_callback_count_++;
  OnHandleReadyInternal(result);
  // At this point, this object might have been deleted.
  if (weak_self) {
    DCHECK_LT(0u, sync_handle_watcher_callback_count_);
    sync_handle_watcher_callback_count_--;
  }
}

void Connector::OnHandleReadyInternal(MojoResult result) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (result != MOJO_RESULT_OK) {
    HandleError(result != MOJO_RESULT_FAILED_PRECONDITION, false);
    return;
  }

  ReadAllAvailableMessages();
  // At this point, this object might have been deleted. Return.
}

void Connector::WaitToReadMore() {
  CHECK(!paused_);
  DCHECK(!handle_watcher_);

  handle_watcher_.reset(new SimpleWatcher(
      FROM_HERE, SimpleWatcher::ArmingPolicy::MANUAL, task_runner_));
  if (heap_profiler_tag_)
    handle_watcher_->set_heap_profiler_tag(heap_profiler_tag_);
  MojoResult rv = handle_watcher_->Watch(
      message_pipe_.get(), MOJO_HANDLE_SIGNAL_READABLE,
      base::Bind(&Connector::OnWatcherHandleReady, base::Unretained(this)));

  if (rv != MOJO_RESULT_OK) {
    // If the watch failed because the handle is invalid or its conditions can
    // no longer be met, we signal the error asynchronously to avoid reentry.
    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&Connector::OnWatcherHandleReady, weak_self_, rv));
  } else {
    handle_watcher_->ArmOrNotify();
  }

  if (allow_woken_up_by_others_) {
    EnsureSyncWatcherExists();
    sync_watcher_->AllowWokenUpBySyncWatchOnSameThread();
  }
}

bool Connector::ReadSingleMessage(MojoResult* read_result) {
  CHECK(!paused_);

  bool receiver_result = false;

  // Detect if |this| was destroyed or the message pipe was closed/transferred
  // during message dispatch.
  base::WeakPtr<Connector> weak_self = weak_self_;

  Message message;
  const MojoResult rv = ReadMessage(message_pipe_.get(), &message);
  *read_result = rv;

  if (nested_dispatch_enabled_) {
    // When supporting nested dispatch, we have to rearm the Watcher immediately
    // after reading each message (i.e. before dispatch) to ensure that the next
    // inbound message can trigger OnHandleReady on the nested loop.
    handle_watcher_->ArmOrNotify();
  }

  if (rv == MOJO_RESULT_OK) {
    receiver_result =
        incoming_receiver_ && incoming_receiver_->Accept(&message);
  }

  if (!weak_self)
    return false;

  if (rv == MOJO_RESULT_SHOULD_WAIT)
    return true;

  if (rv != MOJO_RESULT_OK) {
    HandleError(rv != MOJO_RESULT_FAILED_PRECONDITION, false);
    return false;
  }

  if (enforce_errors_from_incoming_receiver_ && !receiver_result) {
    HandleError(true, false);
    return false;
  }
  return true;
}

void Connector::ReadAllAvailableMessages() {
  while (!error_) {
    base::WeakPtr<Connector> weak_self = weak_self_;
    MojoResult rv;

    // May delete |this.|
    if (!ReadSingleMessage(&rv))
      return;

    if (!weak_self || paused_)
      return;

    DCHECK(rv == MOJO_RESULT_OK || rv == MOJO_RESULT_SHOULD_WAIT);

    if (rv == MOJO_RESULT_SHOULD_WAIT) {
      // Attempt to re-arm the Watcher.
      MojoResult ready_result;
      MojoResult arm_result = handle_watcher_->Arm(&ready_result);
      if (arm_result == MOJO_RESULT_OK)
        return;

      // The watcher is already ready to notify again.
      DCHECK_EQ(MOJO_RESULT_FAILED_PRECONDITION, arm_result);

      if (ready_result == MOJO_RESULT_FAILED_PRECONDITION) {
        HandleError(false, false);
        return;
      }

      // There's more to read now, so we'll just keep looping.
      DCHECK_EQ(MOJO_RESULT_OK, ready_result);
    }
  }
}

void Connector::CancelWait() {
  handle_watcher_.reset();
  sync_watcher_.reset();
}

void Connector::HandleError(bool force_pipe_reset, bool force_async_handler) {
  if (error_ || !message_pipe_.is_valid())
    return;

  if (paused_) {
    // Enforce calling the error handler asynchronously if the user has paused
    // receiving messages. We need to wait until the user starts receiving
    // messages again.
    force_async_handler = true;
  }

  if (!force_pipe_reset && force_async_handler)
    force_pipe_reset = true;

  if (force_pipe_reset) {
    CancelWait();
    internal::MayAutoLock locker(&lock_);
    message_pipe_.reset();
    MessagePipe dummy_pipe;
    message_pipe_ = std::move(dummy_pipe.handle0);
  } else {
    CancelWait();
  }

  if (force_async_handler) {
    if (!paused_)
      WaitToReadMore();
  } else {
    error_ = true;
    if (!connection_error_handler_.is_null())
      connection_error_handler_.Run();
  }
}

void Connector::EnsureSyncWatcherExists() {
  if (sync_watcher_)
    return;
  sync_watcher_.reset(new SyncHandleWatcher(
      message_pipe_.get(), MOJO_HANDLE_SIGNAL_READABLE,
      base::Bind(&Connector::OnSyncHandleWatcherHandleReady,
                 base::Unretained(this))));
}

}  // namespace mojo
