"""Tests of tools/check_firmware_defaults.py: the repository passes, and each way of weakening the settings or a task stack fails."""
import unittest

import check_firmware_defaults as chk

GOOD = """CONFIG_ESP_TASK_WDT_EN=y
CONFIG_ESP_TASK_WDT_TIMEOUT_S=10
CONFIG_ESP_TASK_WDT_PANIC=y
CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0=y
CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1=y
CONFIG_ESP_MAIN_TASK_STACK_SIZE=12288
CONFIG_FREERTOS_CHECK_STACKOVERFLOW_CANARY=y
CONFIG_FREERTOS_USE_TRACE_FACILITY=y
"""
SRC = {"app_main.cpp": 'xTaskCreate(say_task, "say", 8192, nullptr, 3, nullptr);\nxTaskCreate(link_task, "link", 8192, nullptr, 4, nullptr);\nxTaskCreate(stop_task, "stop", 6144, nullptr, 6, &h);\n'}


class FirmwareDefaultsTest(unittest.TestCase):
    def test_the_repository_passes(self):
        self.assertEqual(chk.problems(), [])

    def test_good_settings_pass(self):
        self.assertEqual(chk.problems_of(GOOD, SRC), [])

    def test_each_missing_setting_fails(self):
        for key in chk.REQUIRED_ON:
            text = GOOD.replace(key + "=y\n", "")
            self.assertTrue(any(key in b for b in chk.problems_of(text, SRC)), key)
        self.assertTrue(chk.problems_of(GOOD.replace("TIMEOUT_S=10", "TIMEOUT_S=0"), SRC))
        self.assertTrue(chk.problems_of(GOOD.replace("TIMEOUT_S=10", "TIMEOUT_S=900"), SRC))

    def test_a_small_main_stack_fails(self):
        self.assertTrue(any("MAIN_TASK_STACK" in b for b in chk.problems_of(GOOD.replace("=12288", "=3584"), SRC)))
        self.assertTrue(any("MAIN_TASK_STACK" in b for b in chk.problems_of(GOOD.replace("CONFIG_ESP_MAIN_TASK_STACK_SIZE=12288\n", ""), SRC)))

    def test_a_task_stack_below_its_need_fails(self):
        small = {"app_main.cpp": SRC["app_main.cpp"].replace("8192, nullptr, 4", "4096, nullptr, 4")}
        self.assertTrue(any("'link'" in b and "4096" in b for b in chk.problems_of(GOOD, small)))

    def test_an_unknown_task_or_a_stack_that_is_not_a_literal_fails(self):
        extra = {"app_main.cpp": SRC["app_main.cpp"] + 'xTaskCreate(other_task, "other", 8192, nullptr, 3, nullptr);\n'}
        self.assertTrue(any("'other'" in b and "task_needs" in b for b in chk.problems_of(GOOD, extra)))
        symbolic = {"app_main.cpp": SRC["app_main.cpp"].replace("8192, nullptr, 3", "kBig, nullptr, 3")}
        self.assertTrue(any("literal" in b for b in chk.problems_of(GOOD, symbolic)))

    def test_a_task_in_a_comment_does_not_count(self):
        commented = {"app_main.cpp": SRC["app_main.cpp"] + '// xTaskCreate(old_task, "old", 1024, nullptr, 3, nullptr);\n'}
        self.assertEqual(chk.problems_of(GOOD, commented), [])

    def test_a_task_that_is_no_longer_created_fails(self):
        gone = {"app_main.cpp": SRC["app_main.cpp"].replace('xTaskCreate(stop_task, "stop", 6144, nullptr, 6, &h);\n', "")}
        self.assertTrue(any("'stop'" in b for b in chk.problems_of(GOOD, gone)))


if __name__ == "__main__":
    unittest.main()
