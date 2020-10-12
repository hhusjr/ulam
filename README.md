# ulam
Untyped lambda calculus interpreter implemented by C

短小精悍的无类型lambda演算解释器C语言实现

# 使用方法
g++ ulam.cpp -o ulam

./ulam -o \<Source File Path\>：解释指定源文件中的lambda演算表达式
  
./ulam：进入交互模式

# 语法
## 注释与空白行
“#”号开头行为注释行，空白行忽略
## Lambda表达式
使用严格的lambda表达式：Exp ::== (Exp Exp) | \T.Exp | T

定义助记符号：
\<Symbol Name\>:\<Lambda Expr\>
  
（可以看ex.lamb示例文件）
