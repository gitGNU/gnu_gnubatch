//  Ask host for list of variables we can read

class	listvar	{
private:
	int			pos, nbytes;
	char		buffer[CL_SV_BUFFSIZE];
public:
	listvar(const unsigned = BTM_READ);
	char	*next();
};
