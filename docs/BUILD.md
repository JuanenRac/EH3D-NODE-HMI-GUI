# Building the release firmware

## Building in GitHub Actions

The release images are compiled by GitHub Actions, not on the maintainer's computer: start **Actions > Firmware build (manual)** (`.github/workflows/firmware-build.yml`, `workflow_dispatch` only, never on a push or a pull request). Only the owner of the repository can start it, and the commit must already be part of `main`.

- **Inputs:** `dry_run` (on by default: build and check only), `board` (empty = every `sdkconfig.board.<board>` file of the repository) and `ref` (`main` by default).
- **Raise the version in `eh3d.project.json` before the build.** The job looks for it inside every image and fails if it is not there. The same job fails if the sdkconfig of the build holds a secret; nothing secret is built into an image (the token, the network and the pins are settings of the node).
- **Container:** the official `espressif/idf:v5.5.5` image, pinned by its digest, with no cache of any kind. The build starts from a clean checkout, so no stale `sdkconfig` can get in.
- **Jobs:** `plan` (inputs, owner, commit in `main`), `build` (one job per board), `collect` (`SHA256SUMS`, `build-manifest.json` and a build-provenance attestation), `verify` (the artifact is checked as a stranger would check it), `draft` (only when `dry_run` is off: a draft release with the unsigned files, in its own job that waits for the approval of the owner in the `release` environment) and `notify`.
- **Files:** `<image>-<version>-<board>-app.bin` (the update from the panel) and `<image>-<version>-<board>-complete.bin` (flashed at address 0), in the artifact `firmware-<image>-<version>`. They are **not signed**: the signature is made on the maintainer's computer with the release key, which is never given to Actions.
- **When something fails:** the workflow writes the Job Summary and opens (or updates) an issue labelled `build-failed` that names the job, the step, the reason and the link of the run; a later good build closes it.

The local build (`build.bat`, `build.sh` and the compiler script of the maintainer) stays for development and for flashing a board by cable.
