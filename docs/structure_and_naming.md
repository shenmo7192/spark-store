# 项目结构和命名规范

## 文件夹结构

### cmake

主要用于 CMake 配置时需要用到的 CMake 模块脚本。

### gui

主要用于显示界面的 C++ 代码文件（即包含 Qt Widgets 代码），无论包含多少与其他组件的联系，都在这个目录内。
这包括 SpkUi 的自定义界面组件、商店主界面自身的 Page 类等等。

### inc

项目中所有以 `#include "xxx.h"` 方式包含到代码中的标头文件，全部放入此目录。
标头文件并不强求目录结构明确，文件用途或来源特殊的除外。

### src

商店主体逻辑。包含基础的 `SpkStore` 类、`main.cpp` 等核心逻辑。
新加入的逻辑如单个功能多于一个cpp文件，则必须分装到一个目录内。

### plugin

适配 Deepin 以及其他平台的平台相关插件。

## 命名规范

### 通用规范

类名、成员、方法名一律采用大驼峰形式。例如，`SpkTitleBar::SetTitle` 方法。
C++ 规范要求的和第三方库有较大差异的除外。例如，`SpkUiMessage::_notify`成员。
临时变量采用小驼峰形式或全小写。

名称要对功能要有基础的描述，例如，`SpkUi::SpkCategorySelector` 类。抽象核心逻辑除外。

### 类名

如非第三方代码，一般以 `Spk` 前缀标明。
实现 UI 的类都应归于 `SpkUi` 命名空间。

## 类定义

### 一般规则

Qt 类的 `Q_OBJECT` 应置于类定义最顶端。
`public` ，`protected` 和 `private` 等标签中只应该包含一类成员，
如单个 `public` 标签内只能包含方法，或成员。
信号、槽等不得与普通方法混合。

<!--TODO-->