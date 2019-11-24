while(<>) {
	chomp;
	if (m/^\d+/) {
		print "0x$_,\n";
		next;
	}

	if(m/^ENCODING/){
		print "/** $_ **/\n";
	}
}
