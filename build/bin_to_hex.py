import os, sys, array

data = array.array( 'B' )
data.fromfile( open( sys.argv[1], 'r' ), os.path.getsize(sys.argv[1]) )

count = 1

sys.stdout.write( '0x%02X' % data[0] )

for c in data[1:]:
	if count > 15:
		sys.stdout.write( ',\n0x%02X' % c )
		count = 0
	else:
		sys.stdout.write( ', 0x%02X ' % c )

	count += 1
