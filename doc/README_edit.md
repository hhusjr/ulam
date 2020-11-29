# ulam
Untyped lambda calculus interpreter implemented by C

短小精悍的无类型lambda演算解释器C语言实现

## Lambda演算

$\lambda-calculus$，又称$\lambda$演算，是一套研究函数定义、应用以及函数递归（Recursion）的形式系统。还是函数式编程（Functional Programming）的基础。最有意义的是，$\lambda$演算与图灵机（Turing Machine）等价，这意味着任何一个可计算函数都可以使用$\lambda$演算这种形式来表达和求值。比起图灵机的纸带、方格、机器读写头的硬件模型，$\lambda$演算与软件结合更加紧密。所以，$\lambda$演算还被称为最小的通用程序设计语言。而无类型（Untyped）$\lambda$演算是最简易的$\lambda$演算方式。这个仓库包含仅用500多行C语言代码开发的一个最小而完整的$\lambda$演算解释器。

虽然一个纯粹的lambda演算解释器没有什么实际意义，因为功能有限而且很难产生副作用（Side effect），难以用它实现IO操作，并且性能较低。但是对于理论计算机科学的研究还是非常有意义的。如果需要可以直接在真实项目中使用的函数式编程语言，需要考虑使用Haskell等成熟的现代函数式编程语言。

## 语法

这里大致说明支持的语法格式。后面会通过一些例子来说明。

### 按行执行

对于非空白行，ulam按行执行。每行是一个Lambda Term，或者一个助记符定义语句，或者输出模式定义语句，或者为文件引入语句，或者为注释语句。

### 标准$\lambda$演算语法支持

支持扩展（可省略部分括号）的$\lambda-calculus$文法。唯一的区别在于$\lambda$符号，为了防止乱码或者输入麻烦等问题，替换该符号为反斜杠（$\backslash$）。这里使用$BNF$文法描述为：
$$
term\rightarrow\backslash\ <VARIABLE>\ .\ term \\
term\rightarrow\ applicationTerm \\
applicationTerm\rightarrow atom\ applicationTermExtra \\
applicationTermExtra\rightarrow atom\ applicationTermExtra \\
applicationTermExtra\rightarrow \epsilon \\
atom\rightarrow <VARIABLE> \\
atom\rightarrow (\ term\ ) \\
$$
其中VARIABLE是一个由不包含括号字符、空字符、反斜杠\、点.的任意字符构成的终结符。该文法是一个$LL(1)$文法。为了便于查看结果和便于计算，ulam还支持一些扩展语法。下面会提到。

### 助记符定义

$$
Mnemonic\rightarrow T:Term
$$

其中$T$不能由美元符号$\$$开头。可以把一些$\lambda$ Term定义为一个助记符（Mnemonic）。可以在定义之后直接引用。

ulam还包含一些预定义（Pre-defined）助记符。目前支持生成邱奇数。预定义助记符使用美元符号$\$$开头。$in表示生成n对应的邱奇数的Lambda表达式。后面会有详细的例子来说明。

### 引入其他文件

$$
FileImport\rightarrow :FilePath
$$

冒号后面直接写路径即可。

### 输出模式

助记符不会产生输出。默认情况下，Term会输出化简到最简化形式的Lambda Term。对于一些特殊的Lambda Term，可以定义输出模式：
$$
ModDefinedTerm\rightarrow $\ Modifier:Term
$$
其中，Modifier目前支持

+ i：将Term认作一个邱奇数，并输出对应的整数值。
+ b：将Term认作一个逻辑真/假，并输出true/false。
+ s：将Term化简的每一步推导过程输出。

### 注释

注释单独成行写在# ... #之中即可。

## 求值策略

首先定义

+ $(\backslash x.M)\ N\rightarrow M[N/x]$为$Beta$步骤。
+ $\frac{N->N'}{M N->M N'}$为$Mu$步骤。
+ $\frac{M->M'}{M N->M' N}$为$Nu$步骤。

后面将会引入求值策略动态切换的功能。目前需要自己修改程序来设定求值策略。在$eval\_single\_step$函数中修改$beta\_step(m)\ ||\ nu\_step(m)\ ||\ mu\_step(m)$的顺序即可。常见的求值策略有三种：

+ 懒求值（或者称普通求值顺序，Normal-order）：Beta、Nu、Mu
+ 即时求值（类似于现代编程语言）：Nu、Mu、Beta或Mu、Nu、Beta。

ulam默认使用懒求值策略。

## 使用方法

```shell
g++ ulam.cpp -o ulam # 使用其它编译器也可以
./ulam -o <Source File Path> #解释指定源文件中的lambda演算表达式
./ulam #进入交互模式
```

## 标准库
标准库（std.lamb）包含了常见数学运算和逻辑运算、谓词的lambda演算实现，以及递归函数的核心——Y不动点组合子（Y fixed point combinator）。目前支持（只要学习过Lambda演算，就应该认识下面的这些常见的lambda演算表达式）

```
# ulam standard library #
# By Junru Shen, Hohai University #

# Boolean values #
true:  \a.\b.a
false: \a.\b.b

# Number values #
->: \n.\f.\x.f (n f x)
<-: \n.\f.\x.n (\g.\h.h (g f)) (\u.x) (\u.u)

# Numeric operators #
+: \g.\h.h -> g
-: \g.\h.h <- g
*: \g.\h.h (+ g) $i0
^: \g.\h.h (* g) $i1

# Logical operators #
&: \x.\y.x y x
|: \x.\y.x x y
!: \p.p false true

# Predicates #
empty: \n.n (\x.false) true

# Statements #
if: \p.\a.\b.p a b

# Y fixed point combinator #
Y: \f.(\x.f (x x)) (\x.f (x x)))
```

## 一些栗子

### 递归的阶乘

使用现代编程语言实现递归求阶乘非常非常简单。但是，Lambda演算本身是不支持递归的，接触过Lambda演算的人都应该知道Lambda演算中的函数无法直接调用自身。此时需要使用到Y不动点组合子。下面介绍如何构造阶乘函数

首先，先考虑递归的阶乘的数学表示方法
$$
factorial(n)=1\ if\ n\ ==\ 0\ else\ n\times factorial(n-1)
$$
下面构造与其等价的Lambda演算方式。如果定义一个助记符factorial，即可表示为

```
factorial: \x.if (empty x) $i1 (* x (factorial (<- x)))
```
上面就是ulam的语法形式。这里，$\$i1$是预定义助记符，就表示1对应的邱奇数，也就是$\backslash f.\backslash x.f\ x$。$empty$是一个std.lamb中的助记符，其定义为
$$
empty: \backslash n.n\ (\backslash x.false)\ true
$$
这是一个谓词（Predicate），用来判断一个邱奇数是否为零。

但是这样表示是不允许的。因为，factorial是一个助记符。解释器将直接将它替换为对应的内容，而不会考虑其语义。这里，factorial的定义中，使用了factorial，然而在使用的时候尚未定义过factorial。这就是为什么Lambda演算不支持递归的原因，因为它本身就没有原生的对于递归结构的支持。

所以需要转换思路。我们需要将factorial自身，作为一个参数，传入factorial的定义中，也就是

```
factorial: \f.\x.if (empty x) $i1 (* x (f (<- x)))
```

但是这样，我们就不能通过$(factorial\ \$i5)$这种方式来计算阶乘了，而是需要首先传入一个参数f。而这个f我们希望是$(factorial\ f)$。这里有点考验智商的感觉，也是Lambda演算最精妙的几个地方之一，需要仔细体会其含义。也就是说为了求得一个函数$f$，我们需要列出方程
$$
f=(factorial\ f)
$$
我们定义$f$为$factorial$函数的不动点（Fixed Point）。实际上，f是可以构造出来的。

考虑一个函数Y，

```
Y: \f.(\x.f (x x)) (\x.f (x x)))
```

计算一下就可以发现，$(Y\ g)=(g\ (Y\ g))$。这里的Y是数理逻辑学家Curry Haskell发现的一个Lambda Term，被称为Y不动点组合子。而这个组合子恰好满足刚才的方程$f=(factorial f)$。只需要取
$$
f=(Y factorial)
$$
作为参数传入$factorial$的第一个参数即可。

Y函数已经包含在std.lamb中。

所以，要实现递归阶乘，只需要写下列代码：

```
# 引入std.lamb标准库（路径可能不一样，自己修改下） #
:../lib/std.lamb
# 写出高阶的阶乘递归函数 #
factorial: \f.\x.if (empty x) $i1 (* x (f (<- x)))
# 传入Y不动点组合子，写出阶乘函数 #
factorial_calc: \x.factorial (Y factorial) x
# 计算一个阶乘5!试试看，以邱奇数的模式输出 #
$i:factorial_calc $i5
```

将上述程序保存为一个文件（比如ex.lamb），然后用ulam运行

```shell
ulam -o ex.lamb
```

输出为120。

