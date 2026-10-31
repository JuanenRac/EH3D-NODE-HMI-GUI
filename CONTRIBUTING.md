# Contributing to EH3D-NODE-HMI-GUI

Thank you for looking. This screen is meant to control a machine one day, so changes are small and carefully tested.

1. **Nothing that lets the screen move a machine changes without proof.** A change in the gate (`core/gate.hpp`), in the settings that open a safety switch or in the way a move is sent needs a test that covers it, including the refusals, and a note of what was observed on hardware once there is any.
2. **Every behaviour has a test**, including the negative ones: a value out of range, a stop that is still latched, a spoken word that only contains an order, a missing language text.
3. **Before a pull request:** the host tests (`cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host`) and `python -I tools/ci_validate.py` must pass.
4. **Texts exist in eight languages** (English, Spanish, Italian, German, French, European Portuguese, Chinese and Japanese) wherever the screen shows, says or listens for text. The README files keep the same structure.
5. **Every tuned value is a setting** in `hmi.cfg`, with its default in `hmi.cfg.example`; a safety switch starts in its conservative position.
6. **No files of other people's software.** Do not add programs, manuals or demo code of other makers; the board support comes from the component registry.
7. Commits are written in English, small and about one thing.
