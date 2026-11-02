# EH3D-NODE-HMI-GUI - what each FreeRTOS task of the firmware needs at least (tools/check_firmware_defaults.py reads it; a task that is not here fails the check).
# Every number is the size it has now or more: shrinking one has to be a decision, not an accident. They were not measured on a module yet: the firmware logs the stack each task has
# left (main/app_main.cpp) so these numbers can be tightened or raised after a run on the board.
# Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
NEEDS = {
    "say": (8192, "plays a recorded notice: the samples of a whole file are in memory"),
    "link": (8192, "every request to the server, the JSON of the fleet and the copies of the job"),
    "stop": (6144, "the stop: short requests with three tries"),
}

# The settings of sdkconfig.defaults that keep the screen from freezing or corrupting memory: (key, the least value or the exact one).
MAIN_TASK_STACK_MIN = 8192
WDT_TIMEOUT_MAX_S = 30
