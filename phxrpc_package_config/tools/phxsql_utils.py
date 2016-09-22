def format_path( str ):
	while( str.find( '//' ) != -1 ):
		str = str.replace( '//', '/' )
	return str
