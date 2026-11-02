# Windows 编辑、GitHub Actions 编译

源码基线：官方 `v1.17.0`，提交 `d6f12ad1c4f70ad3230afd7d86e971421e02fef4`。
本地编辑分支：`codex/px4-v1.17.0`。
飞控编译目标：`px4_fmu-v6c_default`。

## Windows 使用

在 VS Code 中打开此目录，直接编辑 `src/`、`boards/`、`ROMFS/` 等源码。
本仓库已配置 `core.autocrlf=false`、`core.longpaths=true`，并通过上游
`.gitattributes` 保持源码 LF 换行。VS Code 的自动 CMake 配置和运行前编译已关闭。
不需要安装 WSL、Docker、ARM 工具链或本地编译依赖。
上游 `.vscode/tasks.json` 仍提供本地构建任务；本工作方式不运行这些任务。

## 已连接的 GitHub 仓库

- 个人 fork：<https://github.com/cbcb224/PX4-Autopilot>
- `origin`：`https://github.com/cbcb224/PX4-Autopilot.git`
- `upstream`：`https://github.com/PX4/PX4-Autopilot.git`
- 默认分支与本地跟踪分支：`codex/px4-v1.17.0`
- 云端工作流：`.github/workflows/firmware-build.yml`
- 构建入口：<https://github.com/cbcb224/PX4-Autopilot/actions/workflows/firmware-build.yml>
- 首次构建：<https://github.com/cbcb224/PX4-Autopilot/actions/runs/41434388335>

工作流、Windows VS Code 设置和本说明文件均在该分支维护。
浏览器登录与本机 Git 身份验证独立。后续第一次 `git push` 可能需要在 Git
Credential Manager 中登录 `cbcb224`，并通过 `git config user.name` 和
`git config user.email` 设置提交署名。

## 编译与下载

1. 在仓库 **Actions → Build PX4 Firmware → Run workflow** 中选择源码分支。
2. 当前默认 target 为 `px4_fmu-v6c_default`。目标由 `boards/<厂商>/<板卡>/<配置>.px4board`
   对应为 `<厂商>_<板卡>_<配置>`，例如
   `boards/px4/fmu-v6c/default.px4board` 对应 `px4_fmu-v6c_default`。
3. 点击 Run workflow。GitHub 的 Ubuntu runner 会在上游开发镜像中检出完整 Git 历史
   和固定版本的递归子模块，然后执行 `make <target>`。
4. 成功后在该次运行的 Artifacts 下载 `firmware-<target>-<编号>`，解压取得固件。
   `logs-<编号>-<尝试次数>` 保存编译日志与源码版本，失败时也会尝试上传。
   Artifacts 保存 30 天。

工作流只手动触发。无需 PX4 的 AWS runner 或发布密钥。
上游自带工作流仍保留，部分依赖 PX4 专用 runner/密钥；本流程使用
`firmware-build.yml`。推送到 `main`、`stable`、`beta`、`release/**` 或 `v*` 标签，
以及创建 PR，可能触发上游工作流。

## 后续编辑

```powershell
git status
git add <修改过的文件>
git commit -m "Describe your change"
git push
```

每次提交后手动运行上述工作流，确认选择了包含修改的分支。
如果修改的是子模块内部源码，需要先将子模块提交推送到可访问的仓库、更新
`.gitmodules`（如使用自己的 fork）并提交主仓库中的子模块指针；仅推送主仓库
不会上传子模块的未提交修改。
