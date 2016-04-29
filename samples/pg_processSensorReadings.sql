 	CREATE OR REPLACE FUNCTION processSensorReadings(sequenceNumber text)
 	RETURNS text
 	AS $$
 	DECLARE
 	  parm text;
 	  msg json;
 	  shardIter text ;
 	  data text ;
 	  time text;
 	  temp text ;
 	BEGIN
 	  SELECT '{"StreamName": "MySensorReadings","SequenceNumber": "' || sequenceNumber || '"}' INTO parm ;
 	  LOOP
 	      SELECT CAST(awslambda_fn('lambdaReadKinesisStream', parm) as JSON) INTO msg ;
 	      SELECT msg->'NextShardIterator' INTO shardIter;
 	      SELECT TRIM(BOTH '"' FROM shardIter) INTO shardIter; --remove quotes
 	      SELECT msg->'Data' INTO data ;
 	      EXIT WHEN data = '""' ;  -- message is empty - exit the loop
 	      SELECT msg->'SequenceNumber' INTO sequenceNumber;
 	      SELECT msg->'Data'->'time' INTO time;
 	      SELECT msg->'Data'->'temp' INTO temp;
 	      RAISE NOTICE 'Time %, Temperature %', time, temp ;
 	      SELECT '{"StreamName": "MySensorReadings","NextShardIterator": "' || shardIter || '"}' INTO parm ;
 	      -- Do something with the data here - maybe insert into a table?
 	  END LOOP;
 	  RETURN sequenceNumber;
 	END ;
 	$$ LANGUAGE plpgsql;
