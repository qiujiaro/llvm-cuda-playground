# Daily Logs

One file per day. Each is pre-filled with that day's goals, reading list, questions,
and acceptance criteria from [`10-Day_Hands-On_Project_Guide.md`](../../10-Day_Hands-On_Project_Guide.md)
— you fill in what actually happened.

| Day | Theme | File | Status |
|----|------|------|--------|
| 1 | Environment & LLVM back-end mental model | [day-01.md](day-01.md) | ⬜ |
| 2 | LLVM IR traversal & dependency extraction | [day-02.md](day-02.md) | ⬜ |
| 3 | Scheduling DAG + latency model + critical path | [day-03.md](day-03.md) | ⬜ |
| 4 | List Scheduler core | [day-04.md](day-04.md) | ⬜ |
| 5 | Live interval & register pressure | [day-05.md](day-05.md) | ⬜ |
| 6 | Linear-Scan Register Allocator | [day-06.md](day-06.md) | ⬜ |
| 7 | Benchmark corpus + harness | [day-07.md](day-07.md) | ⬜ |
| 8 | Run experiments + compare vs LLVM MachineScheduler | [day-08.md](day-08.md) | ⬜ |
| 9 | Charts / dashboard / tests / CI | [day-09.md](day-09.md) | ⬜ |
| 10 | README / report / resume / demo | [day-10.md](day-10.md) | ⬜ |

Blank template for any extra/overflow day: [TEMPLATE.md](TEMPLATE.md)

## How to use these

1. **Morning:** read section 0–2, do the reading, answer the questions *in your own words*
   before writing code. If you can't answer them, you're not ready to implement.
2. **While coding:** fill section 3 as you go — especially "hand-written vs vibe-coded."
   That column is what you defend in an interview.
3. **After running:** fill section 4 with **real numbers only**. Leave blank rather than guess.
4. **End of day:** fill the bug table and section 9, the interview soundbite. If you can't
   write the soundbite, you didn't understand what you built.

## Rules

- **Never pre-fill a result.** No number goes in until a command produced it.
- **Log failures.** The bug table is the most valuable part for interviews — "what went
  wrong and how I diagnosed it" is a better story than "it worked."
- **Mark SKIPs honestly.** If a test still skips because a module isn't done, say so.
- Commit the day's log with the day's code, so the git history and the narrative match.
