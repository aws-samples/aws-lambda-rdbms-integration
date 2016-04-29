CALL dbms_java.grant_permission( 'SYSTEM', 'SYS:java.lang.RuntimePermission', 'getenv.AWS_PROFILE', '');
CALL dbms_java.grant_permission( 'SYSTEM', 'SYS:java.lang.RuntimePermission', 'accessDeclaredMembers', '' );
CALL dbms_java.grant_permission( 'SYSTEM', 'SYS:java.lang.RuntimePermission', 'getClassLoader', '' );
CALL dbms_java.grant_permission( 'SYSTEM', 'SYS:java.lang.reflect.ReflectPermission', 'proxyConstructorNewInstance', '' );
CALL dbms_java.grant_permission( 'SYSTEM', 'SYS:java.net.SocketPermission', '*', 'connect,resolve' );
