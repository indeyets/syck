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
echo "Testing load: ";
print_r( syck_load( "{test: &anc {and: 2}, or: {alias: *anc}}" ) );
?>
