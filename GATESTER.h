// A library of Mathematical functions to test a genetic algorithm
// <cmath> library must be imported

float schwefel (float *argv, int dim)
{
	float value = 418.9829f*(float)dim;
	for (int i=0; i<dim; i++)
	{
		value -= argv[i]*sin(sqrt(fabs(argv[i])));
	}
	return value;
}

// shifted Schwefel function: standard range [0, 1000]
double sh_schwefel (std::vector<double> vals)
{
	int dim = vals.size();
	double value = 418.9829*(double)dim;
	
	double tmp;	
	for (int i=0; i<dim; i++)
	{
		tmp = vals[i] - 500.0;	// shifting args	
		value -= tmp*sin(sqrt(fabs(tmp)));
	}
	return value;
}

// Schwefel function: standard range [-500,500]
double schwefel (std::vector<double> vals)
{
	int dim = vals.size();
	double value = 418.9829*(double)dim;
	
	double tmp;	
	for (int i=0; i<dim; i++)
	{
		tmp = vals[i];	
		value -= tmp*sin(sqrt(fabs(tmp)));
	}
	return value;
}

// Schwefel function with region [0,50] of INFINITY value
double inf_schwefel (std::vector<double> vals)
{
	// region limits
	double LOW = 0.0;
	double HIGH = 50.0;
	for (std::vector<double>::iterator it=vals.begin(); it!=vals.end(); it++)
	{
		if (*it>=LOW && *it<=HIGH)
			return INFINITY;
	}

	int dim = vals.size();
	double value = 418.9829*(double)dim;
	
	double tmp;	
	for (int i=0; i<dim; i++)
	{
		tmp = vals[i];	
		value -= tmp*sin(sqrt(fabs(tmp)));
	}
	return value;
}

// negative Schwefel function: standard Schwefel - 100
double neg_schwefel (std::vector<double> vals)
{
	int dim = vals.size();
	double value = 418.9829*(double)dim;
	
	double tmp;	
	for (int i=0; i<dim; i++)
	{
		tmp = vals[i];	
		value -= tmp*sin(sqrt(fabs(tmp)));
	}
	return value - 100;		// shifting output by const
}

// Schwefel function with region [0,50] of very large value (not INFINITY)
double lge_schwefel (std::vector<double> vals)
{
	// region limits
	double LOW = 0.0;
	double HIGH = 50.0;
	for (std::vector<double>::iterator it=vals.begin(); it!=vals.end(); it++)
	{
		if (*it>=LOW && *it<=HIGH)
			return 999999999.9999;	// the very large value
	}

	int dim = vals.size();
	double value = 418.9829*(double)dim;
	
	double tmp;	
	for (int i=0; i<dim; i++)
	{
		tmp = vals[i];	
		value -= tmp*sin(sqrt(fabs(tmp)));
	}
	return value;
}