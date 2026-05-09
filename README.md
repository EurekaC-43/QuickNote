# QuickNote - 费曼笔记生成器

基于 Qt 的桌面笔记工具，用费曼学习法生成 Markdown 笔记。

## 功能

- 输入项目名称、遇到的问题、自己的理解
- 自动生成带 frontmatter 的 Markdown 文件
- 自定义输出目录，支持设为默认
- 右侧实时 Markdown 预览，可切换源码/渲染模式

## 构建

```bash
# 需要安装 Qt6
brew install qt

# 编译
qmake quicknote.pro
make -j4

# 运行
open quicknote.app
```

## 取消 .app 打包

在 `quicknote.pro` 中取消注释：

```qmake
CONFIG -= app_bundle
```

则输出为裸可执行文件 `quicknote`。

## 项目结构

```
quicknote/
├── main.cc          # 全部源码（单文件）
├── quicknote.pro     # qmake 项目文件
└── Makefile          # qmake 生成
```
