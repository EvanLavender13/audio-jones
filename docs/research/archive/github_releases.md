# GitHub Actions Release Pipeline

Automated build-and-release pipeline: push a named tag, GitHub Actions builds the exe, zips it with runtime assets, and publishes a GitHub Release with a changelog.

## Classification

- **Type**: General — CI/CD infrastructure
- **Chosen Approach**: Balanced — single Windows zip per tag, static CRT, changelog from repo

## References

- [GitHub Actions: CMake + MSVC + Ninja setup](https://github.com/marketplace/actions/setup-ninja) - Ninja binary install action
- [Create Release + Upload Artifacts pattern](https://trstringer.com/github-actions-create-release-upload-artifacts/) - Tag-triggered release workflow
- [softprops/action-gh-release](https://github.com/softprops/action-gh-release) - Modern release action with file glob support

## Workflow Design

### Trigger

Any tag push. No version format enforced — tags are fun names like `SMOOTHBOB`, `GALACTO`.

```yaml
on:
  push:
    tags: ['*']
```

### Build Steps

1. Checkout repo
2. Install Ninja via `ashutoshvarma/setup-ninja`
3. Configure MSVC environment via `ilammy/msvc-dev-cmd` (sets up `cl.exe` on PATH)
4. CMake configure: `-G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded`
5. CMake build
6. Assemble staging directory: `AudioJones.exe` + `shaders/` + `presets/` + `fonts/`
7. Zip the staging directory

### Release Steps

1. Extract the latest changelog entry from `CHANGELOG.md` (everything between the first `## tag-name` header and the next `##` header)
2. Create GitHub Release using `softprops/action-gh-release` with:
   - Release name = tag name
   - Body = extracted changelog section
   - Artifact = `AudioJones-<tag>.zip`

### Static CRT

`CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded` links the C runtime statically (`/MT`). The exe runs on any Windows machine without needing the Visual C++ Redistributable installed.

## CHANGELOG.md Format

```markdown
# Changelog

## SMOOTHBOB

- Add physarum simulation with trail diffusion
- Fix bloom threshold clamping at low values
- New preset: GALACTO

## BINGBANG

- Initial release
- 70 effects, 7 simulations, 21 presets
```

Each section uses the tag name as the header. The workflow extracts the top section for the release description.

### Changelog Workflow

1. Accumulate commits on main
2. Ask Claude to review commits since last tag and draft a changelog entry
3. Commit the updated `CHANGELOG.md`
4. Tag and push: `git tag SMOOTHBOB && git push origin SMOOTHBOB`
5. GitHub Actions builds and publishes automatically

## Files to Create

| File | Purpose |
|------|---------|
| `.github/workflows/release.yml` | The Actions workflow |
| `CHANGELOG.md` | Changelog with per-release sections |

## Notes

- `windows-latest` runners have MSVC 2022 pre-installed — no manual compiler setup beyond `msvc-dev-cmd`
- FetchContent downloads all dependencies during CMake configure — no git submodules needed
- First build on CI takes ~3-5 min due to dependency fetching; GitHub caches help on subsequent runs
- Free tier: 2000 min/month for public repos — each release build uses ~5 min
- The zip excludes `build/`, `docs/`, `.claude/`, `.git/` — only runtime assets ship
