
/* SYNCHRONOUSLY CALL NAMED LAMBDA FUNCTION */
CREATE OR REPLACE FUNCTION awslambda_fn(fn_name VARCHAR2, fn_argsJson VARCHAR2) 
    RETURN VARCHAR2
    AS LANGUAGE JAVA
    NAME 'com.amazonaws.rdbmsinteg.LambdaInvoke.invoke_sync(java.lang.String, java.lang.String) return java.lang.String';
/
GRANT EXECUTE ON awslambda_fn TO PUBLIC;
/
CREATE OR REPLACE PUBLIC SYNONYM awslambda_fn FOR awslambda_fn;
/

/* ASYNCHRONOUSLY CALL NAMED LAMBDA FUNCTION */
CREATE OR REPLACE FUNCTION awslambda_fn_async(fn_name VARCHAR2, fn_argsJson VARCHAR2) 
    RETURN VARCHAR2
    AS LANGUAGE JAVA
    NAME 'com.amazonaws.rdbmsinteg.LambdaInvoke.invoke_async(java.lang.String, java.lang.String) return java.lang.String';
/
GRANT EXECUTE ON awslambda_fn_async TO PUBLIC;
/
CREATE OR REPLACE PUBLIC SYNONYM awslambda_fn_async FOR awslambda_fn_async;
/

/* STORED PROCEDUDRE TO ASYNCHRONOUSLY CALL NAMED LAMBDA FUNCTION */
CREATE OR REPLACE PROCEDURE awslambda_proc(fn_name VARCHAR2, fn_argsJson VARCHAR2)
    AS LANGUAGE JAVA
    NAME 'com.amazonaws.rdbmsinteg.LambdaInvoke.invoke_async(java.lang.String, java.lang.String)';
/
GRANT EXECUTE ON awslambda_proc TO PUBLIC;
/
CREATE OR REPLACE PUBLIC SYNONYM awslambda_proc FOR awslambda_proc;
/

