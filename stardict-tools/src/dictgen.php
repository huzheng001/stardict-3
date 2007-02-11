#!/usr/bin/php
<?php
#####  Begin of Setting  ##########
	$output_file='f2002';
	$input_mode='txt';	//txt or xml
	$input_file='0.txt';
	##  Begin of Info ####
		$GLOBALS['bookname']='F2002_French_to_Chinese';	//required
		$GLOBALS['author']='tonykuo';
		$GLOBALS['email']='tonykuo@mail.tcm.ncku.edu.tw';
		$GLOBALS['website']='http://residence.educities.edu.tw/french/F2002.htm';
		$GLOBALS['description']='Converted by gmobug@gmail.com';
		$GLOBALS['date']='2006.06.06';
	##  End Info ####
	##  Begin of XML input setting ####
		$GLOBALS['wt']='word';	//Word tag
		$GLOBALS['et']='text';	//Explaint tag
	##  End of XML input setting ####
#####  End of Setting  ##########
$GLOBALS['phase']=1;
####  Begin of function declaration  ##########
function stardict_input(){
	echo "\nPhase ".$GLOBALS['phase']++." : Input\n";
	$bom=chr(239).chr(187).chr(191);
	echo "\nInput type [txt/xml/dict]: ";
	$ans=rtrim(fgets(STDIN));
	if($ans=='txt'){
		echo "\nFilename: ";
		$fn=rtrim(fgets(STDIN));
		while(!file_exists($fn)){
			echo "\nFile not found!";
			echo "\nFilename: ";
			$fn=rtrim(fgets(STDIN));
		}
		echo "\nAre you sure input file use UTF-8 charset?[yN]";
		$ans=rtrim(fgets(STDIN));
		if($ans!='y'){
			die("\nAbort!\n");
		}
		$s=file_get_contents($fn);
		if(substr($s,0,3)==$bom){
			$ans='';
			echo "\nRemove UTF-8 BOM?[Yn]";
			$ans=rtrim(fgets(STDIN));
			if($ans!='n'){
			$s=substr($s,3);
			}
		}
		$s=trim($s);
		$s=str_replace("\r\n","\n",$s);
		$s=explode("\n",$s);
		for($i=0;$i<count($s);$i++){
			$pos=strpos($s[$i],"\t");
			if($pos==FALSE){
				die("\nNo definition!\nAbort!\n");
			}
			$z[0][$i]=substr($s[$i],0,$pos);
			$z[1][$i]=substr($s[$i],$pos+1);
		}
		for($i=0;$i<count($s);$i++){
			$z[1][$i]=deslash($z[1][$i]);
		}
		$GLOBALS['dict']=$z;
	}elseif($ans=='xml'){
		$wt=$GLOBALS['wt'];
		$et=$GLOBALS['et'];
		echo "\nFilename: ";
		$fn=rtrim(fgets(STDIN));
		while(!file_exists($fn)){
			echo "\nFile not found!";
			echo "\nFilename: ";
			$fn=rtrim(fgets(STDIN));
		}
		echo "\nAre you sure input file use UTF-8 charset?[yN]";
		$ans=rtrim(fgets(STDIN));
		if($ans!='y'){
			die("\nAbort!\n");
		}
		$s=file_get_contents($fn);
		preg_match_all('/<'.$wt.'>.*?<\/'.$wt.'>/s',$s,$a);
		preg_match_all('/<'.$et.'>.*?<\/'.$et.'>/s',$s,$b);
		if(count($a[0])!=count($b[0])){
			die("\nWord number != Explain number\nAbort!\n");
		}
		if(count($a[0])==0){
			die("\nNo Data Found!\nAbort!\n");
		}
		for($i=0;$i<count($a[0]);$i++){
			$a[0][$i]=str_replace('<'.$wt.'>','',$a[0][$i]);
			$a[0][$i]=str_replace('</'.$wt.'>','',$a[0][$i]);
			$a[0][$i]=trim($a[0][$i]);
			$b[0][$i]=str_replace('<'.$et.'>','',$b[0][$i]);
			$b[0][$i]=str_replace('</'.$et.'>','',$b[0][$i]);
			$b[0][$i]=trim($b[0][$i]);
		}
		$GLOBALS['dict']=array($a[0],$b[0]);
	}elseif($ans=='dict'){
		echo "\nFilename (Suffix is not needed): ";
		$fn=rtrim(fgets(STDIN));
		while(!file_exists($fn)){
			echo "\nFile not found!";
			echo "\nFilename (Suffix is not needed): ";
			$fn=rtrim(fgets(STDIN));
		}
		####
	}else{
		die("\nUnknown input mode\n");
	}
}
function stardict_cmp($a,$b){
	$z=strcasecmp($a,$b);
	if($z!=0){
		return $z;
	}else{
		return strcmp($a,$b);
	}
}
function stardict_sort(){
	$z=$GLOBALS['dict'];
	echo "\nPhase ".$GLOBALS['phase']++." : Sort\n";
	$a=$z[0];
	$b=$z[1];
	for($i=0;$i<count($a);$i++){
		$m[$a[$i]]=$b[$i];
	}
	usort($a, 'stardict_cmp');
	for($i=0;$i<count($a);$i++){
		$b[$i]=$m[$a[$i]];
	}
	$GLOBALS['dict']=array($a,$b);
}
function stardict_nbo($n){
#Network Byte Order
	$s1=$n & 255;
	$n=$n>>8;
	$s2=$n & 255;
	$n=$n>>8;
	$s3=$n & 255;
	$n=$n>>8;
	$s4=$n & 255;
	return chr($s4).chr($s3).chr($s2).chr($s1);
}
function stardict_diacritic(){
	$z=$GLOBALS['dict'];
	echo "\nPhase ".$GLOBALS['phase']++." : Diacritic Assistant\n";
	$char_from=array('à','â','è','é','ê','ë','î','ï','ô','ö','ù','û','ü','ç');
	$char_to = array('a','a','e','e','e','e','i','i','o','o','u','u','u','c');
	$pat=implode('|',$char_from);
	$a=$z[0];
	$b=$z[1];
	for($i=0;$i<count($a);$i++){
		if(preg_match('/('.$pat.')/',$a[$i])){
			$tmp=$a[$i];
			for($j=0;$j<count($char_from);$j++){
				$tmp=str_replace($char_from[$j],$char_to[$j],$tmp);
			}
			if(in_array($tmp,$a)){
				$key=array_search($tmp,$a);
				$b[$key].="\n\n==============================\nDiacritic Assistant: ".$a[$i];
			}else{
				$a[]=$tmp;
				$b[]='Diacritic Assistant: '.$a[$i];
			}
		}
	}
	$GLOBALS['dict']=array($a,$b);
}
function stardict_uniq(){
	$z=$GLOBALS['dict'];
	$tmp=$z[0][0];
	$t[0][0]=$z[0][0];;
	$t[1][0]=$z[1][0];;
	for($i=1;$i<count($z[0]);$i++){
		if($z[0][$i]==$tmp){
			$t[1][count($t[1])-1].='\n------------------------------\n'.$z[1][$i];
		}else{
			$tmp=$z[0][$i];
			$t[0][]=$z[0][$i];
			$t[1][]=$z[1][$i];
		}
	}
	$GLOBALS['dict']=$t;
}
function deslash($s){
	$z='';
	for($i=0;$i<strlen($s);$i++){
		if(substr($s,$i,1)=='\\'){
			$k=substr($s,$i+1,1);
			if($k=='n'){
				$z.="\n";
				$i++;
			}elseif($k=='t'){
				$z.="\t";
				$i++;			
			}else{
				$z.='\\';
			}
		}else{
			$z.=substr($s,$i,1);
		}
	}
	return $z;
}
function enslash($s){
	$z='';
	for($i=0;$i<strlen($s);$i++){
		$k=substr($s,$i,1);
		if($k=='\\'){
			$z.='\\\\';
		}elseif($k=="\n"){
			$z.='\\n';
		}else{
			$z.=$k;
		}
	}
	return $z;
}
function stardict_output(){
	$z=$GLOBALS['dict'];
	echo "\nPhase ".$GLOBALS['phase']++." : Output\n";
	echo "\nOutput type [txt/xml/dict]: ";
	$ans=rtrim(fgets(STDIN));
	if($ans=='txt'){
		echo "\nFilename: ";
		$output_file=rtrim(fgets(STDIN));
		if(file_exists($output_file)){
			echo "\nOutput files already exist!\nOverwrite?[Yn]";
			$ans='';
			$ans=rtrim(fgets(STDIN));
			if($ans=='n'){
				die("\nAbort!\n");
			}		
		}
		$a=fopen($output_file,"w+");
		fwrite($a,$z[0][0]."\t".enslash($z[1][0]));
		for($i=1;$i<count($z[0]);$i++){
			fwrite($a,"\n".$z[0][$i]."\t".enslash($z[1][$i]));
		}
		$ans='';
		echo "\nAdd new line at the end of file?[yN]";
		$ans=rtrim(fgets(STDIN));
		if($ans=='y'){
			fwrite($a,"\n");
		}
		fclose($a);
	}elseif($ans=='dict'){
		echo "\nFilename (Suffix is not needed): ";
		$output_file=rtrim(fgets(STDIN));
		if(file_exists($output_file.'.idx') || file_exists($output_file.'.dict') || file_exists($output_file.'.ifo')){
			echo "\nOutput files already exist!\nOverwrite?[Yn]";
			$ans='';
			$ans=rtrim(fgets(STDIN));
			if($ans=='n'){
				die("\nAbort!\n");
			}
		}
		$idx=fopen($output_file.'.idx',"w+");
		$dict=fopen($output_file.'.dict',"w+");
		$pos=0;
		for($i=0;$i<count($z[0]);$i++){
			fwrite($dict,$z[1][$i]);
			$len=strlen($z[1][$i]);
			fwrite($idx,$z[0][$i].chr(0).stardict_nbo($pos).stardict_nbo($len));
			$pos+=$len;
		}
		fclose($idx);
		fclose($dict);
		$ifo=fopen($output_file.'.ifo',"w+");
		fwrite($ifo,"StarDict's dict ifo file\n");
		fwrite($ifo,"version=2.4.2\n");
		fwrite($ifo,'wordcount='.count($z[0])."\n");
		fwrite($ifo,'idxfilesize='.filesize($output_file.'.idx')."\n");
		fwrite($ifo,'bookname='.$GLOBALS['bookname']."\n");
		if(isset($GLOBALS['author']) && strlen($GLOBALS['author'])>0){
			fwrite($ifo,'author='.$GLOBALS['author']."\n");
		}
		if(isset($GLOBALS['email']) && strlen($GLOBALS['email'])>0){
			fwrite($ifo,'email='.$GLOBALS['email']."\n");
		}
		if(isset($GLOBALS['website']) && strlen($GLOBALS['website'])>0){
			fwrite($ifo,'website='.$GLOBALS['website']."\n");
		}
		if(isset($GLOBALS['description']) && strlen($GLOBALS['description'])>0){
			fwrite($ifo,'description='.$GLOBALS['description']."\n");
		}
		if(isset($GLOBALS['date']) && strlen($GLOBALS['date'])>0){
			fwrite($ifo,'date='.$GLOBALS['date']."\n");
		}
		fwrite($ifo,"sametypesequence=m\n");
		fclose($ifo);
	}else{
		die('Unknown.');
	}
	echo "\nDone!\n";
}
####  End of function declaration  ##########
$flow='#Input';
stardict_input();
$mode=0;
while($mode!=6){
	echo "\n".$flow."\n";
	echo "Proccess:\n";
	echo "1. Input\n";
	echo "2. Diacritic Assistant\n";
	echo "3. Sort\n";
	echo "4. Unique\n";
	echo "5. Output\n";
	echo "6. Quit\n";
	$mode=rtrim(fgets(STDIN));
	switch($mode){
		case 1: stardict_input(); $flow.=' => Input'; break;
		case 2: stardict_diacritic(); $flow.=' => Diacritic'; break;
		case 3: stardict_sort(); $flow.=' => Sort'; break;
		case 4: stardict_uniq(); $flow.=' => Unique'; break;
		case 5: stardict_output(); $flow.=' => Output'; break;
		case 6: exit(); break;
		case 9527: echo "\nHaha! No kidding :)\n"; break;
		default: continue;
	}
}
?>