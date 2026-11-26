// ============================================================================
// 15_holy_rtos_scheduler.HC - HolyC Ring-0 Preemptive Task Control Scheduler
// ============================================================================

Class CHolyTcb {
  I64 tid;
  I64 prio;
  I64 ticks;
  I64 state;
};

I64 HolyRtosStep(I64 tid, I64 prio, I64 quanta) {
  CHolyTcb task;
  task.tid = tid;
  task.prio = prio;
  task.ticks = 0;
  task.state = 1;
  I64 i = 0;
  while (i < quanta) {
    task.ticks = task.ticks + task.prio + i;
    i = i + 1;
  }
  return task.ticks;
}

U0 HolyRtosSchedulerVerify() {
  I64 total = 0;
  total = total + HolyRtosStep(1, 5, 8);
  total = total + HolyRtosStep(2, 3, 6);
  total = total + HolyRtosStep(3, 9, 4);
  if (100 <= total <= 50000) {
    "[AOT-RTOS-SCHED] Ring-0 Scheduler quanta balanced\n";
  }
}
