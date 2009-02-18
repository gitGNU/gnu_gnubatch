#define	MAXFORMATS	30

class Formatrow {
public:
	CString	f_field;		//  Hdr or separator
	UINT	f_code;			//  Resource code
	UINT	f_width;		//  Width of field
	UINT	f_maxwidth;		//  Widest we've met
	char	f_char;			//  Char in resource string
	BOOL	f_issep;		//  Separator
	BOOL	f_ltab;			//  Left tab
	BOOL	f_skipright;	//  Forget right
	BOOL	f_rjust;		//  Right-justify header
	int		(*f_fmtfn)(...);//	Format function
};
