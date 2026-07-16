import unittest

from backend.main import _ir_metrics, _parse_pass_timings, _parse_remarks


class IrAnalysisTests(unittest.TestCase):
    def test_counts_instructions_and_basic_blocks(self) -> None:
        ir = """
define i32 @answer(i1 %condition) {
entry:
  br i1 %condition, label %yes, label %no
yes:
  ret i32 42
no:
  ret i32 0
}
"""
        self.assertEqual(_ir_metrics(ir), (3, 3))

    def test_parses_llvm_pass_timing_row(self) -> None:
        stderr = "  0.0001 ( 10.0%)  0.0000 ( 0.0%)  0.0001 ( 10.0%)  0.0002 ( 20.0%)  InstCombinePass"
        timings = _parse_pass_timings(stderr)
        self.assertEqual(len(timings), 1)
        self.assertEqual(timings[0].name, "InstCombinePass")
        self.assertEqual(timings[0].time_ms, 0.2)

    def test_parses_optimization_remark(self) -> None:
        stderr = "input.cpp:9:5: remark: vectorized loop [-Rpass=loop-vectorize]"
        remarks = _parse_remarks(stderr)
        self.assertEqual(len(remarks), 1)
        self.assertEqual(remarks[0].kind, "passed")
        self.assertEqual(remarks[0].pass_name, "loop-vectorize")
        self.assertEqual(remarks[0].line, 9)


if __name__ == "__main__":
    unittest.main()
