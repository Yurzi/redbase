# RedBase

这是斯坦福大学 CS346 Spring 公开课的RedBase代码，咱自己对代码结构进行了重新编排，调整了一些东西包括PF模块。
同时咱还重建了项目结构，将makefile工程重建为了cmake工程，并使用ctest作为测试组件。

现在还在编写中……🤤

## 如何构建

首先， 在项目根目录下创建`build`目录

```sh
mkdir build
```
然后，配置cmake工程

```sh
cmake -B build -G Ninja   # 这里咱使用ninja buildsystem，如果汝要使用make，可以不加 -G及后参数
```

最后，使用构建系统构建

```sh
ninja -C build

# 如果你使用make，可以通过以下命令构建
# make -C build
#
```

## 如何测试

首先，若项目根目录下不存在`build`目录，则创建

```sh
mkdir build
```
然后，配置cmake工程

```sh
cmake -B build -G Ninja -DENABLE_TEST=ON    # 这里咱使用ninja buildsystem，如果汝要使用make，可以不加 -G及后参数
                                            # 将ENABLE_TEST 设为 ON
```

最后，使用构建系统构建

```sh
ninja -C build

# 如果你使用make，可以通过以下命令构建
# make -C build
#
```

最后，运行测试

```sh
ctest --test-dir build
```

## 添加测试

详见CMakeLists.txt
