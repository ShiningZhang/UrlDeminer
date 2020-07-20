# UrlDeminer
2019搜狗编程比赛url过滤
参赛代码

给定4个输入：URL域名过滤规则， URL前缀过滤规则，待匹配的URL，URL文件特征。 用户程序接收4个命令行参数，要求选手编写程序，输出URL的过滤结果到标准输出。
Cpu8核/内存12G
1. 域名过滤规则文件

域名规则文件每一行一个域名规则，格式如下：
    DOMAIN_FILTER_KEY TAB PERMISSION
例子：

abc.com -
abc.cn  +
.abc.com    -
abc.com.cn:8080 +
PERMISSION只能是"+"或者"-"，"+"表示通过过滤器(passed)，"-"表示被过滤掉(filtered)。

域名过滤的基本原则是带端口的规则高于不带端口的规则，长的规则高于短的规则。


2. URL前缀过滤规则文件

前缀黑名单文件每一行一个域名黑名单，前缀黑名单的格式如下：
    URL_PREFIX_FILTER_KEY TAB RANGE TAB PERMISSION
例子：

https://www.abc.com/index.html  =   -
http://www.abc.com/images/  *   +
https://www.abc.com/    +   -
//www.abc.com/index.html?opt=   +   +
URL_PREFIX_FILTER_KEY的格式要求见下文。

RANGE只能是"=+*"中的一个。"="表示要求URL精确匹配此条规则，"+"表示要求URL
前缀符合此条规则，且URL长于此前缀，"*"表示表示要求URL前缀符合此条规则或精确匹配
此规则。

PERMISSION只能是"+"或者"-"，"+"表示通过过滤器，"-"表示被过滤掉。

URL前缀过滤的基本原则是长的规则高于短的规则，端口除了:80和:443的规约以外需要严格
匹配。冲突的规则以"-"优先。

4.1 域名匹配规则

DOMAIN_FILTER_KEY 的定义如下：
DOMAIN_FILTER_KEY = DOMAINPORT | "." DOMAINPORT | TOP_DOMAIN | "." TOP_DOMAIN
DOMAINPORT 的定义见上文。
取出URL的domain:port部分进行匹配
如果url的scheme是http，且没有指定端口，则端口视作80 
如果url的scheme是https，且没有指定端口，则端口视作443


以"."作为分割点，取包含自身在内的所有的后缀，检查是否与规则匹配。如果有若干匹配，取命中的最长的后缀规则的PERMISSION作为匹配结果。
若1没有结果，则忽略:port部分，重复步骤1。
同样长度的规则有冲突，则"-"优先。
如果没有任何规则命中，则认为匹配结果是通过。


例子：

======
基本的例子：
规则：
www.baike.com.cn    +
.baike.com.cn   -
baike.com.cn    +
效果：
http://www.baike.com.cn/ => ALLOWED
https://news.baike.com.cn/ => DISALLOWED
http://news.baike.com.cn/ => DISALLOWED
http://news.baike.com.cn:8080/ => DISALLOWED
https://baike.com.cn/ => ALLOWED
=======
有端口优先于无端口的例子:
规则：
www.798.com.cn  +
.798.com.cn:8080    -
效果：
http://www.798.com.cn/ => ALLOWED
http://www.798.com.cn:8080/ => DISALLOWED
http://news.798.com.cn:8080/ => DISALLOWED
=======
80端口和443端口的特殊情况
80端口的规则默认对以http://开头的URL生效。
443端口的规则默认对以https://开头的URL生效。
例子：
规则：
.mi.com.cn  -
.mi.com.cn:80   +
效果：
http://www.mi.com.cn/ => ALLOWED
https://www.mi.com.cn/ => DISALLOWED
http://www.mi.com.cn:8080/ => DISALLOWED
======
规则有冲突，则"-"优先。
例子：
规则：
www.emacs.com.cn    -
www.emacs.com.cn    +
效果：
http://www.emacs.com.cn/ => DISALLOWED

4.2 前缀匹配规则

URL_PREFIX_FILTER_KEY 的定义：
URL_PREFIX_FILTER_KEY = SCHEME "://" DOMAINPORT PATH | "//" DOMAINPORT PATH
SCHEME, DOMAINPORT, PATH的定义见上文。
以"//"开头的前缀规则匹配SCHEME部分任意的URL。在本次比赛中，可以视为分解为SCHEME为http和https的两条规则。
"// DOMAINPORT PATH => 
  "http://" DOMAINPORT PATH 
  "https://" DOMAINPORT PATH 

首先将URL处理为正规形式。


如果URL以某一条规则的URL_PREFIX_FILTER_KEY为前缀，且满足该条规则RANGE的要求，则视为匹配。
存在多条匹配的规则时，取URL_PREFIX_FILTER_KEY最长的规则的PERMISSION作为最终匹配结果。
如果多条匹配规则的URL_PREFIX_FILTER_KEY一致，"*"的优先级低，RANGE为"="或者"+"都优先于"*"。(不存在同时匹配两条URL_PREFIX_FILTER_KEY一致，但是RANGE分别为=和+的情况。)
规则有重复，则"-"优先于"+"。
如果没有任何规则命中，则认为匹配结果是通过。


例子：

======
基本的例子：URL_PREFIX_FILTER_KEY的长度优先。
规则：
http://news.gcc.com.cn/about    *   -
http://news.gcc.com.cn/a    +   +
http://news.gcc.com.cn/ *   -
效果：
http://news.gcc.com.cn/about.html => DISALLOWED
http://news.gcc.com.cn/about => DISALLOWED
http://news.gcc.com.cn/abc => ALLOWED
http://news.gcc.com.cn/a => DISALLOWED
http://news.gcc.com.cn/ => DISALLOWED
http://news.gcc.com.cn/copyright.html => DISALLOWED
======
两条规则URL_PREFIX_FILTER_KEY一致，RANGE不一致，"+"比"*"优先。
规则：
http://news.sohu.com.cn/about   *   -
http://news.sohu.com.cn/about   +   +
效果：
http://news.sohu.com.cn/about.html => ALLOWED
http://news.sohu.com.cn/about => DISALLOWED
======
两条规则URL_PREFIX_FILTER_KEY一致，RANGE不一致，"="比"*"优先。
规则：
http://news.163.com.cn/about    *   -
http://news.163.com.cn/about    =   +
效果：
http://news.163.com.cn/about.html => DISALLOWED
http://news.163.com.cn/about => ALLOWED
======
两条规则URL_PREFIX_FILTER_KEY一致，RANGE一致，PERMISSION"-"优先。
规则：
http://news.zi.com.cn/about *   -
http://news.zi.com.cn/about *   +
效果：
http://news.zi.com.cn/about.html => DISALLOWED
======
scheme不同视为不匹配，http和https不混淆
规则：
http://news.gl.com.cn/about *   -
效果：
http://news.gl.com.cn/about.html => DISALLOWED
https://news.gl.com.cn/about.html => ALLOWED
======
端口不同视为不匹配：
规则：
http://news.ruby.com.cn:8080/about  *   -
效果：
http://news.ruby.com.cn/about.html => ALLOWED
https://news.ruby.com.cn:8080/about.html => ALLOWED
======
允许规则或者输入URL强制指定与默认端口不一致的行为，此时匹配原则仍然是scheme和port要一致：
规则：
https://news.java.com.cn/about  *   -
效果：
https://news.java.com.cn:80/about.html => ALLOWED
======
//开头的URL_PREFIX_FILTER_KEY视做http和https两个scheme规则的简写
规则：
//news.perl.com.cn/about    *   -
效果：
http://news.perl.com.cn/about.html => DISALLOWED
https://news.perl.com.cn/about.html => DISALLOWED
规则：
//news.julia.com.cn/about   *   -
https://news.julia.com.cn/a +   +
效果：
http://news.julia.com.cn/about.html => DISALLOWED
https://news.julia.com.cn/about => DISALLOWED

5. CASE的数据规模

mapreduce问题
规则匹配有很多，包括port优先，最长优先，-优先，然后=>+>*（规则同等长度下）
1、我的思路：domain保存长度、字符、port、规则，并且按照规则（+-）、末尾字符是否为.com,.cn普通划分，然后按照末尾两字符换分，并且记录每个域名的相等范围（a,ab,abc,b:(1,2,3,1)）
前缀按照=、（+*)划分，然后按照http和https区分，同样是头两个字符（+www.）划分，
然后匹配的时候先匹配domain，找到不大于url的第一个domain，如果range<8,逐个匹配，否则依次将url长度减一，然后进行二分查找。
匹配prefix时，同样按划分查找对应表，然后按规则优先度找，找到不大于url的规则，然后逐个匹配
最终成绩1800s
2、头名思路（300s）：
使用hash，选用BKDRHash对domain、prefix（host、path）、url作计算，然后在hash中进行查找
~~~c++
//BKDRHash seed 31 131 1313 13131 131313...
static inline uint64_t BKDRHash(const char *str, size_t slen)
{
    uint64_t hash = 0;

    while (slen--)
        hash = hash * 131 + *str++;

    return hash;
}
~~~
domain规则实际索引范围
    端口号 + BKDRHash(host) + host尾字符奇偶
    16bit + 64bit + 1bit = 81bit

    prefix规则path为空的实际索引范围
    scheme类型 + 端口号 + BKDRHash(host)
    1bit + 16bit + 64bit = 81bit

    (case2/final2)prefix规则path不为空的实际索引范围
    scheme类型 + 端口号 + 混淆128bit为64bit(BKDRHash(host), BKDRHash(path))
    1bit + 16bit + 64bit = 81bit

    (case3/case4/final3/final4)prefix规则path不为空实际索引范围
    scheme类型 + 端口号 + BKDRHash(host) + BKDRHash(path)去掉头2位 + path首字符奇偶 + path长度是否大于24
    1bit + 16bit + 64bit + 62bit + 1bit + 1bit = 145bit
对于小量url，用词典树来进行处理，
对于大量url，就用hash建立，然后直接查找。
难点：压缩、查找
