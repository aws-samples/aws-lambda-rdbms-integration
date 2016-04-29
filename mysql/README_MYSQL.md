NOTES for Enabling Lambda invocation on MySQL
----------------------------------------
This implementation has *not* yet been well tested.. It is provided as-is - please use at your own risk!

Instance Type for build
-------------------------
Use Amazon Linux - c4.2xlarge instance (smaller images my fail building the AWS C++ SDK)
EC2 image must be launched with EC2 instance role as described in blog (necessary for SDK to use temporary credentials)


Install MySQL 5.6  on Amazon Linux
-----------------------------------
```
sudo yum install mysql56-server mysql56 mysql-devel
sudo service mysqld start
```

Install C++ and Cmake
----------------------
```
sudo yum install gcc-c++  curl-devel libarchive-devel bzip2-devel expat-devel xz-devel
wget http://www.cmake.org/files/v3.5/cmake-3.5.0.tar.gz
tar xvzf cmake-3.5.0.tar.gz
cd cmake-3.5.0
./bootstrap --prefix=/usr --system-libs  --mandir=/share/man  --no-system-jsoncpp --docdir=/share/doc/cmake-3.5.0
make
```

Install AWS SDK for C++
-----------------------
```
git clone https://github.com/aws/aws-sdk-cpp.git
cd aws-sdk-cpp
cmake .
make
sudo make install
sudo cp /usr/local/lib/linux/intel64/Release/libaws-cpp-sdk-{lambda,core}.so /lib64/
```

Build and Install UDFs for MySQL
--------------------------------
```
git clone https://github.com/rstrahan/rdbms-lambda-integ.git
cd rdbms-lambda-integ/mysql/src
cmake .
make
sudo make install

mysql –u root
mysql> CREATE FUNCTION awslambda_fn RETURNS STRING SONAME "libawslambda_fn.so";
Query OK, 0 rows affected (0.00 sec)
mysql> CREATE FUNCTION awslambda_fn_async RETURNS STRING SONAME "libawslambda_fn.so";
Query OK, 0 rows affected (0.00 sec)
```

(NOTE the FUNCTION DDL is also in rdbms-lambda-integ/mysql/sql/awslambda_ddl.sql

Test the UDFs using AWS Lambda function 'lambdaTest' (see blog)
----------------------------------------------------------------

```
mysql> SELECT awslambda_fn('lambdaTest','{"name":"bob"}') AS lambdatest ;
+--------------------------------------------+
| lambdatest                                 |
+--------------------------------------------+
| {
   "Status" : "OK",
   "name" : "bob"
}
 |
+--------------------------------------------+
1 row in set (0.31 sec)

mysql> SELECT awslambda_fn_async('lambdaTest','{"name":"bob"}') AS lambdatest ;
+------------+
| lambdatest |
+------------+
| {
}
       |
+------------+
1 row in set (0.22 sec)
```

