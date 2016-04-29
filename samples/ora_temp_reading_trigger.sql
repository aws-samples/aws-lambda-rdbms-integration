 	CREATE OR REPLACE TRIGGER temp_reading_trigger 
    AFTER INSERT 
    ON temp_reading 
    FOR EACH ROW
    DECLARE
        fn_name varchar2(32) := 'lambdaTempReadingTrigger';
        fn_args varchar2(255);
        t varchar2(32);
        v number;
        res varchar2(32);
    BEGIN
        t := TO_CHAR(SYS_EXTRACT_UTC(:new.reading_time),'YYYY-MM-DD HH24:MI:SS');
        v := :new.reading_value;
        fn_args := '{"reading_time":"' || t || '", "reading_value":"' || v || '"}' ;
        DBMS_OUTPUT.PUT_LINE('Calling: ' || fn_name || ', input: ' || fn_args);
        SELECT awslambda_fn_async(fn_name, fn_args) INTO res FROM DUAL;
    END;
    /
