# 无类型$\lambda-calculus$概述与解释器构建

## 引言

$\lambda-calculus$，又称$\lambda$演算，是一套研究函数定义、应用以及函数递归（Recursion）的形式系统。最有意义的是，$\lambda$演算与图灵机（Turing Machine）等价，这意味着任何一个可计算函数都可以使用$\lambda$演算这种形式来表达和求值。比起图灵机的纸带、方格、机器读写头的硬件模型，$\lambda$演算与软件结合更加紧密。所以，$\lambda$演算还被称为最小的通用程序设计语言。而无类型（Untyped）$\lambda$演算是最简易的$\lambda$演算方式。本文将先介绍$\lambda$演算的概念与文法规则，然后探索如何从零开始，设计并使用C语言开发一个最小而完整的$\lambda$演算解释器。最后介绍并使用该解释器演示函数式编程中的高阶函数、Y不动点组合子、邱奇数等特征。

## $\lambda-calculus$基本概念

在$\lambda$演算中，首先需要了解的概念是符号（Symbol）。$\lambda$演算仅仅包含下列符号（这些符号的作用在后面解释）：

+ $\lambda$

+ $.$

+ 表明运算优先级的括号
+ 变量名（x，y，a，b等等）

$\lambda$演算只有两种核心的语法。所有你在现代编程语言中的纷繁复杂的if、for语句，甚至TRUE/FALSE布尔类型字面量、数字字面量等等，都可以使用$\lambda$演算的符号和语法进行编码。

### 函数抽象（Abstraction）

$$
\lambda x.x
$$

上面的公式代表一个函数抽象（Abstraction）。我们可以将它想象成一个输入x，输出x的函数。$\lambda$表示后面跟着的是一个形式参数。而.起到分隔的作用，.之前相当于形式参数的定义，.之后相当于函数的“返回值”。当然，这只是暂时理解。真正的$\lambda-calculus$中，是没有返回值这种概念的。后面具体描述。

如果类比Python里的这种写法，可能就很好理解了：

```python
lambda x: x
```

没错，Python里的lambda就是来源于$\lambda-calculus$。但是Python里的lambda演算不是纯净（Pure）的。真正的lambda演算是不支持加号之类的运算符，针对数字字面量、加法等，都有一套单独的编码方式。

### 作用（Application）

仅有函数抽象，而没有“作用”于某一个对象是没有意义的。我们如何表示将该“函数”作用于一个变量呢？很简单：
$$
(\lambda x.x\ y)
$$
 类比Python：

```python
(lambda x: x)(y)
```

按照Python的理解，y作为实参传入函数抽象$\lambda x.x$，函数返回$y$。实际上，lambda演算中的理解有点小区别。$\lambda-calculus$中，强调“替换”。$(\lambda x.x\ y)$表示用y替换掉所有的x。当然，替换掉之后，“$\lambda x.$”就没有了。所以可以记为
$$
(\lambda x.x\ y)=y
$$
举一些其他的例子。

* $(\lambda x.x\ (\lambda y.y\ z))=(\lambda x.x\ z)=z$。这个里面有两层Application运算。逐层计算即可。
* $(\lambda x.y\ z)=y$。这里需要注意，在左边的函数抽象$\lambda x.y$中，主体（$.$右边的部分）为$y$，而作用操作是将所有$\lambda x.y$里面的$x$替换为$y$。相当于在Python里面写了一句(lambda x: y)(z)。这样实际上得到的结果是和$z$没有关系的。
* $(\lambda x.(x\ x)\ \lambda x.(x\ x))$。事情变得有些复杂了。这个读者可以自己计算一下，会发现计算将永无止境地进行下去。$(\lambda x.(x\ x)\ \lambda x.(x\ x))=(\lambda x.(x\ x)\ \lambda x.(x\ x))=(\lambda x.(x\ x)\ \lambda x.(x\ x))=(\lambda x.(x\ x)\ \lambda x.(x\ x))=\cdots$
* $(\lambda x.f(x\ x)\ \lambda x.f(x\ x))$。这个就更加有趣了。

