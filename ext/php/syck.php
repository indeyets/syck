<?
if(!extension_loaded('syck')) {
	dl('syck.so');
}
$module = 'syck';
$functions = get_extension_funcs($module);
echo "Functions available in the test extension:<br>\n";
foreach($functions as $func) {
    echo $func."<br>\n";
}
echo "<br>\n";
echo "Testing load: \n";
$doc =<<<YAML
- 
  a: 1
  b: 2
  c: 3
  d: TRUE
  e: ~
  f: test_me
  g:
    test: also
  h: 0xFF
  i: 012
YAML;

$iter = 1000;

echo "DOC #1 = $iter x " . strlen( $doc ) . "\n";
$syck_start = microtime();
foreach( range( 0, $iter ) as $i )
{
    $test_obj = syck_load( $doc );
}
$syck_stop = microtime();

$test_str = serialize( $test_obj );
$unser_start = microtime();
foreach( range( 0, $iter ) as $i )
{
    $test_obj = unserialize( $test_str );
}
$unser_stop = microtime();

$doc2 = "";
foreach( range( 0, $iter ) as $i )
{
    $doc2 .= $doc . "\n";
}
echo "DOC #2 = 1 x " . strlen( $doc2 ) . "\n";
$syck2_start = microtime();
$test_obj = syck_load( $doc2 );
$syck2_stop = microtime();

$test_str = serialize( $test_obj );
$unser2_start = microtime();
$test_obj = unserialize( $test_str );
$unser2_stop = microtime();


function elapsed( $start, $stop )
{
    $start_mt = explode( " ", $start );
    $stop_mt = explode( " ", $stop );
    $start_total = doubleval( $start_mt[0] ) + $start_mt[1];
    $stop_total = doubleval( $stop_mt[0] ) + $stop_mt[1];
    return sprintf( "%0.6f", $stop_total - $start_total );
}

echo "syck:  " . elapsed( $syck_start, $syck_stop ) . " " . elapsed( $syck2_start, $syck2_stop ) . "\n";
echo "php:   " . elapsed( $unser_start, $unser_stop ) . " " . elapsed( $unser2_start, $unser2_stop ) . "\n";

?>
