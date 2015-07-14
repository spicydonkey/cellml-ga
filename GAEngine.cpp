#include "GAEngine.h"

bool reverse_compare(const Genome& v1,const Genome& v2) 
{
	return (v1<v2); 
}

Genome::Genome():m_Fitness(0.0),m_Valid(true)
{
}

Genome::Genome(const Genome& other):m_Fitness(other.m_Fitness),m_Valid(other.m_Valid)
{
	m_Alleles.assign(other.m_Alleles.begin(),other.m_Alleles.end());
}

Genome::~Genome()
{
}

Genome Genome::operator=(const Genome& other)
{
	if(&other!=this)
	{
		m_Alleles.assign(other.m_Alleles.begin(),other.m_Alleles.end());
		m_Fitness=other.m_Fitness;
		m_Valid=other.m_Valid;
	}
	return *this;
}

double Genome::allele(const std::wstring& name)
{
	ALLELE::iterator it=find_if(m_Alleles.begin(),m_Alleles.end(),
								bind1st(pair_equal_to<std::wstring,double>(),name));
	return (it==m_Alleles.end()?double(0.0):it->second);
}

double Genome::allele(int index)
{
	return ((index>=0 && index<m_Alleles.size())?m_Alleles[index].second:0.0);
}

void Genome::allele(const std::wstring& name,double val)
{
	ALLELE::iterator it=find_if(m_Alleles.begin(),m_Alleles.end(),
								bind1st(pair_equal_to<std::wstring,double>(),name));
	if(it!=m_Alleles.end())
		it->second=val;
	else
		m_Alleles.push_back(std::make_pair<std::wstring,double>(std::wstring(name),double(val)));
}

void Genome::allele(int index,double val)
{
	if(index>=0 && index<m_Alleles.size())
		m_Alleles[index].second=val;
}

std::wstring Genome::name(int index)
{
	return ((index>=0 && index<m_Alleles.size())?m_Alleles[index].first:std::wstring());
}

std::pair<std::wstring,double>& Genome::operator[](int index)
{
	while(m_Alleles.size()<=index)
		m_Alleles.push_back(std::make_pair(std::wstring(),double(0.0)));
	return m_Alleles[index];
}

bool Genome::operator<(const Genome& other) const
{
	if(valid() && other.valid())
		return fitness()<other.fitness();
	return valid();
}

bool Genome::operator>(const Genome& other) const
{
    if(valid() && other.valid())
        return fitness()>other.fitness();
    return valid();
}

bool Genome::operator==(const Genome& other) const
{
    if(valid() && other.valid())
        return fitness()==other.fitness();
    return false;
}

bool Genome::same(const Genome& other) const
{
	if(!(valid() && other.valid()))
		return false;

	if(other.m_Alleles.size()!=m_Alleles.size())
        return false;
    for(int i=0;i<m_Alleles.size();i++)
    {
        if(m_Alleles[i].first!=other.m_Alleles[i].first ||
		   m_Alleles[i].second!=other.m_Alleles[i].second)
            return false;
    }
    return true;
}

void Genome::var(VariablesHolder& v)
{
	// iterate through the alleles in this genome
    for(ALLELE::iterator it=m_Alleles.begin();it!=m_Alleles.end();++it)
    {
		// update each allele in v
        v(it->first,it->second);
    }
}

void Genome::set(VariablesHolder& v)
{
	// rebuild alleles from that stored in a varholder
    m_Alleles.clear();	// clear m_Alleles vector

    for(int k=0;;k++)
    {
		std::wstring name=v.name(k);
		if(name.empty())	// k reached the end of m_Var in v
			break;

        m_Alleles.push_back(std::make_pair<std::wstring,double>(name,v(name)));		// append a pair made from m_Vars of v to m_Alleles
    }
}


// TODO How to override class definition by template
template<class COMP>
GAEngine<COMP>::GAEngine():m_MaxPopulation(0),m_Generations(1),
					m_CrossProbability(0.2),m_MutationProbability(0.01),
					m_bBestFitnessAssigned(false),
					m_crossPartition(0),m_mutatePartition(0),
					m_RNG(0)
{
}

template<class COMP>
GAEngine<COMP>::~GAEngine()
{
}
 
template<class COMP>
void GAEngine<COMP>::set_borders(int max_population)
{
	m_MaxPopulation=max_population;
	m_Population.resize(max_population);
}

// Initialise the population with randomly generated genomes
template<class COMP>
bool GAEngine<COMP>::Initialise()
{
	// Create a representative genome for the population
	Genome v;

	// exit exceptions
	if(!m_Population.size() || !m_AlleleList.size())
		return false;

	// Attach alleles to the representative genome
	for(typename std::vector<std::wstring>::iterator it=m_AlleleList.begin();it!=m_AlleleList.end();++it)	// iterate through the list of alleles
	{
		// Create each allele with all values initialised to 0.0
		v.allele(*it,(double)0.0);
	}

	// Fill the population with each mutation of the genome
	for(int i=0;i<m_Population.size();i++)
	{
		mutate(std::wstring(),v,true);	// mutate all the alleles of v
		m_Population[i]=v;				// a randomly generated genome enters the population
	}

	return true;
}

template<class COMP>
void GAEngine<COMP>::AddAllele(const std::wstring& name)
{
    m_AlleleList.push_back(name);
}

template<class COMP>
void GAEngine<COMP>::AddLimit(const std::wstring& name,double lower,double upper)
{
    m_Limits[name]=std::make_pair(double(lower),double(upper));
}

template<class COMP>
void GAEngine<COMP>::var_template(VariablesHolder& v)
{
    for(std::vector<std::wstring>::iterator it=m_AlleleList.begin();it!=m_AlleleList.end();++it)
    {
        v(*it,0.0);
    }
}

template<class COMP>
double GAEngine<COMP>::GetBest(VariablesHolder& v)
{
    v=m_bestVariables;
    return m_bestFitness;
}

template<class COMP>
WorkItem *GAEngine<COMP>::var_to_workitem(VariablesHolder& h)
{
    WorkItem *w=new WorkItem;
    w->key=0;	// key initialised to 0
    h.collate(w->data);
    return w;
}

template<class COMP>
void GAEngine<COMP>::process_workitem(WorkItem *w,double answer)
{
    if(w->key<m_Population.size())
    {
        Genome& g=m_Population[w->key];		// get the genome corresponding to this workitem
        g.fitness(answer);	// assign the evaluated fitness to the genome
    }
    delete w;
}

// Run the GA engine for given number of generations
template<class COMP>
void GAEngine<COMP>::RunGenerations(int gener)
{
	VariablesHolder v;		// temporary genome storage
	m_Generations=gener;	// number of generations to run

	// Create initial fitness set
	for(int i=0;i<m_Population.size();i++)
	{
		Genome& g=m_Population[i];	// get the ith Genome in population

		g.var(v);	// store the genomic information in a temporary variable
		WorkItem *w=var_to_workitem(v);		// collate the genome in a workitem for processing
		w->key=i;	// the key stores a reference to genome

		Distributor::instance().push(w);	// push this work onto the distributor singleton
	}

	// Process the work item collected by distributor
	// evaluate and assign fitness for each Genome in population
	Distributor::instance().process(observer,this);

	std::sort(m_Population.begin(),m_Population.end(),reverse_compare);		// sort population in ascending order of fitness

	// Update fittest genome
	// the fittest genome after sorting population is the 0th member
	if(!m_bBestFitnessAssigned || m_bestFitness>m_Population[0].fitness())
	{
		m_bestFitness=m_Population[0].fitness();	// assign min ftns as the best fitness
		m_Population[0].var(m_bestVariables);		// update the bestVars
		m_bBestFitnessAssigned=true;
	}

	print_stage(0);

	for(int g=1;g<=gener;g++)
	{
		// Do the genetics
		int limit=m_Population.size();
		POPULATION prev(m_Population);	// temporary copy of current pop vector
		m_Population.clear();			// clear current population vector

		// SELECTION
		// Weighted-selection with replacement from the previous generation's population
		for(int i=0;i<limit;i++)
		{
			int mem=select_weighted(prev);		// genome's index [p.size()-1 when err]

			// Genetic operator feedback
			if(verbosity>3)
				printf("SELECT: Adding %d to population\n",mem);

			m_Population.push_back(prev[mem]);	// add the selected genome into the new population for breeding
		}

		// Print the new selected population
		if(verbosity>2)
		{
			printf("--------------------------------------------------------\n");
			printf("Selected Population:\n");
			print_population();
			printf("--------------------------------------------------------\n");
		}

		// CROSSOVER
		// Caution: Multiple crossovers allowed in single generation iteration
		if(m_crossPartition)
		{
			// vector of genome indices selected for genetic operations
			std::vector<int> sample;

			// fill sample with unique and valid indices to genomes for performing crossover
			build_rnd_sample_rnd(sample,m_CrossProbability*100.0,true);

			for(int i=0;i<sample.size();i++)
			{
				std::vector<int> arena; // initialise arena for breeding
				arena.push_back(sample[i]);	// sampled genome enters arena 
				//bulid tournament sample
				build_rnd_sample(arena,1,true,true);	// a unique and valid genome enters arena for crossbreeding

				// Genetic operator feedback
				// Output genomes in arena pre-crossover
				if(verbosity>3)
				{
					/* output format
					CROSSOVER
					-[i] x1=_________ x2=__________
					-[j] x1=_________ x2=__________
					*/
					printf("CROSSOVER:\n");
					for(int j=0;j<arena.size();j++)
					{
						printf("-");
						print_genome(arena[j]);
					}
				}

				// cross the genomes in arena before a randomly selected point
				// multiple degree crossover in a single generation iteration possible (i.e. crossover processed genome may be tournament selected for additional crossover)
				cross(m_Population[arena[0]],m_Population[arena[1]],
					  (int)rnd_generate(1.0,m_Population[sample[i]].size()));

				// Output genomes in arena post-crossover
				if(verbosity>3)
				{
					/*
					+[i] x1=_________ x2=__________
					+[j] x1=_________ x2=__________
					-----------------------------------
					*/
					for(int j=0;j<arena.size();j++)
					{
						printf("+");
						print_genome(arena[j]);
					}
					printf("---------------------------------------\n");
				}

				for(int j=0;j<2;j++)
				{
					m_Population[arena[j]].var(v);	// store the Xover operated genome in template
					Distributor::instance().remove_key(arena[j]);	// remove previously requested processing

					// Set-up workitem for Xover'd genome job
					WorkItem *w=var_to_workitem(v);
					w->key=arena[j];
					Distributor::instance().push(w);
				}
				// genomes weight-selected into population that did not undergo Xover do not need to be re-worked for fitness 
			}

			// Print population after crossover
			if(verbosity>2)
			{
				printf("--------------------------------------------------------\n");
				printf("Crossover:\n");
				print_population();
				printf("--------------------------------------------------------\n");
			}
		}

		// MUTATION
		if(m_mutatePartition)
		{
			std::vector<int> sample;

			// Mutation welcomes invalid genomes
			build_rnd_sample_rnd(sample,m_MutationProbability*100.0,false);

			// Treatment of invalid genomes in the population
			for(int i=0;i<m_Population.size();i++)
			{
				// add all unselected invalid genomes into sample
				if(!m_Population[i].valid() && std::find(sample.begin(),sample.end(),i)==sample.end())
					sample.push_back(i);
			}

			// Mutate the selected genomes
			for(int i=0;i<sample.size();i++)
			{
				// Genetic operator feedback
				// Output genomes pre-mutation
				if(verbosity>3)
				{
					/*
					MUTATION:
					-[i] x1=_________ x2=__________
					+[i] x1=_________ x2=__________
					-----------------------------------
					*/
					printf("MUTATION:\n");
					printf("-");
					print_genome(sample[i]);
				}

				mutate(std::wstring(),m_Population[sample[i]],!(m_Population[sample[i]].valid()));	// mutate the whole chromosome iff genome is invalid. else mutate approx 1 allele

				// Output genomes post-mutation
				if(verbosity>3)
				{
					printf("+");
					print_genome(sample[i]);
					printf("---------------------------------------\n");
				}

				m_Population[sample[i]].var(v);
				Distributor::instance().remove_key(sample[i]); //remove previously requested processing
				WorkItem *w=var_to_workitem(v);
				m_Population[sample[i]].set(v);		// TODO may not be necessary 
				w->key=sample[i];
				Distributor::instance().push(w);
			}

			// Print population after mutation
			if(verbosity>2)
			{
				printf("--------------------------------------------------------\n");
				printf("Mutation:\n");
				print_population();
				printf("--------------------------------------------------------\n");
			}
		}

		// Distribute the fitness evaluation for this generation
		Distributor::instance().process(observer,this);
		// Sort the population
		std::sort(m_Population.begin(),m_Population.end(),reverse_compare);

		if(m_Population.size()>m_MaxPopulation)
		{
			//Cull it
			m_Population.erase(m_Population.begin()+m_MaxPopulation,m_Population.end());
		}

		// update best fitness
		if(!m_bBestFitnessAssigned || m_bestFitness>m_Population[0].fitness())
		{
			m_bestFitness=m_Population[0].fitness();
			m_Population[0].var(m_bestVariables);
			m_bBestFitnessAssigned=true;
		}
		print_stage(g);
	}
}

template<class COMP>
void GAEngine<COMP>::print_config(const int gener) const
{
	printf("Genetic Algorithm:\n");
	printf("Generations=%d  Population=%d  MutationRate=%lf  CrossoverRate=%lf  RNG=%d\n",gener,m_MaxPopulation,m_MutationProbability,m_CrossProbability,m_RNG);
	
	// Allele list	
	for(int i=0;i<m_AlleleList.size();i++)
	{
		printf("* %s: [%lf,%lf]\n",convert(m_AlleleList[i]).c_str(),m_Limits[m_AlleleList[i]].first,m_Limits[m_AlleleList[i]].second);
	}
}

template<class COMP>
void GAEngine<COMP>::print_genome(int ind_genome)
{
	VariablesHolder v;
	m_Population[ind_genome].var(v);	// store alleles data in a temporary variable
	printf("[%d] ", ind_genome);		// print the genome's index
	for(int i=0;;i++)
	{
		std::wstring name=v.name(i);
		// sequence all alleles in genome
		if(name.empty())
			break;
		printf("%s=%lf   ",convert(name).c_str(),v(name));
	}
	printf("\n");
}

template<class COMP>
void GAEngine<COMP>::print_population()
{
	int popsize=m_Population.size();
	for(int i=0;i<popsize;i++)
	{
		print_genome(i);
	}
}

template<class COMP>
void GAEngine<COMP>::print_stage(int g)
{
	//verbose summary of GA: print all chromosomes of curr gen
	if(verbosity>1)
	{
		printf("--------------------------------------------------------\n");
		std::cout << currentDateTime() << std::endl;	// tag generation output with timestamp

		for(int j=0;j<m_Population.size();j++)
		{
			//print validity, generation #, and fitness of each chromosome
			printf("%s[%d](%lf) ",(m_Population[j].valid()?(m_Population[j].fitness()<0?"!":" "):"*"),g,m_Population[j].fitness());

			// Sequence the chromosome
			print_genome(j);
		}
		printf("--------------------------------------------------------\n");
	}

	//shorter summary of GA: print currently fittest chromosome
	// and fittest chromosome of this generation
	else if(verbosity==1)
	{
		std::cout << currentDateTime() << std::endl;	// tag generation output with timestamp

		//VariablesHolder v;
		double f;

		// Fittest chromosome in this gen is the first genome in sorted population
		f=m_Population[0].fitness();

		printf("Generation %d. Best fitness: %lf\n",g,f);
		print_genome(0);
		printf("--------------------------------------------------------\n");
	}
}

template<class COMP>
void GAEngine<COMP>::mutate(const std::wstring& name,Genome& g,bool mutate_all)
{
	double prob=(mutate_all?101.0:100.0/g.size());

	for(int i=0;i<g.size();i++)
	{
		double p=rnd_generate(0.0,100.0);

		// mutate if p <= prob
		if(p>prob)		// chance to skip mutation
			continue;

		if(!name.size() || g.name(i)==name)
		{
			LIMITS::iterator it=m_Limits.find(g.name(i));	// check for param limits of this allele
			double val;

			// TODO selection of RNG implementation
			if(m_RNG==0)
			{
				// Default linear RNG
				if(it==m_Limits.end())
				{
					// no limits, just use [-RAND_MAX/2,RAND_MAX/2] as a limit
					val=rnd_generate(-RAND_MAX*0.5,RAND_MAX*0.5);
				}
				else
				{
					// restrict RNG to set limits
					val=rnd_generate(it->second.first,it->second.second);
				}
			}
			else if(m_RNG==1)
			{
				// Log-type RNG
				if(it==m_Limits.end())
				{
					// DEBUG
					std::cerr << "DEBUG: log-type RNG ERROR" << std::endl;

					// TODO find a method for no-limit case
					// no limits, just use [-RAND_MAX/2,RAND_MAX/2] as a limit
					val=rnd_generate(-RAND_MAX*0.5,RAND_MAX*0.5);
				}
				else
				{
					// restrict RNG to the logarithm of set limits (positive definite) and exponentiate to tranform to original scale
					val=exp(rnd_generate(log(it->second.first),log(it->second.second)));
				}
			}

			g.allele(i,val);	// set the RNG value to allele

			if(name.size())		// only mutate this allele if name specified
				break;
		}
	}
}

template<class COMP>
bool GAEngine<COMP>::cross(Genome& one,Genome& two,int crosspoint)
{
	Genome n1,n2;

	// check if genomes' alleles have same size and crosspoint lies in valid range
	if(one.size()!=two.size() || one.size()<crosspoint+1)
		return false;
	// genomes equal size and crosspoint valid

	// swap alleles before the crosspoint
	for(int i=0;i<crosspoint;i++)
	{
		n1[i]=two[i];
		n2[i]=one[i];
	}

	for(int i=crosspoint;i<one.size();i++)
	{
		n2[i]=two[i];
		n1[i]=one[i];
	}

	one=n1;
	two=n2;

	return true;
}

template<class COMP>
void GAEngine<COMP>::build_rnd_sample(std::vector<int>& sample,int count,bool reject_duplicates,bool check_valid)
{
	double limit=(double)m_Population.size()-0.5;

	for(;count>0;count--)
	{
		int v;

		// randomly assign an int to v 
		// if reject_duplicates set true, add a unique index to sample
		// if check_valid set true, add a valid index
		do
		{
			// nested do-while until genome is both valid and unique
			do
			{
				v=(int)(rnd_generate(0.0,limit));
			} while(check_valid && !m_Population[v].valid());
		} while(reject_duplicates && std::find(sample.begin(),sample.end(),v)!=sample.end());
		//Found next genome
		sample.push_back(v);
	}
}

template<class COMP>
void GAEngine<COMP>::build_rnd_sample_tournament(std::vector<int>& sample,int count,bool reject_duplicates,bool check_valid)
{
	double limit=(double)m_Population.size()-0.5;
	count*=2; //create tournament pairs
	int index=sample.size();

	for(;count>0;count--)
	{
		int v;

		do
		{
			v=(int)(rnd_generate(0.0,limit));
			if(check_valid && !m_Population[v].valid())
				continue;
		} while(reject_duplicates && std::find(sample.begin(),sample.end(),v)!=sample.end());
		//Found next value
		sample.push_back(v);
	}
	//let the fight begins!
	for(int i=index;i<sample.size();i++)
	{
		Genome& one=m_Population[sample[i]];
		Genome& two=m_Population[sample[i+1]];

		if(one>two)
			sample.erase(sample.begin()+i);
		else
			sample.erase(sample.begin()+i+1);
	}
}

template<class COMP>
void GAEngine<COMP>::build_rnd_sample_rnd(std::vector<int>& sample,double prob,bool check_valid)
{
	// if check_valid, only appends indices of m_Population for which Genomes are valid, at given probability (%)
	// else (check_valid==false), appends Genomes at given rate (%)
	for(int i=0;i<m_Population.size();i++)
	{
		if((!check_valid || m_Population[i].valid()) && prob>=rnd_generate(0.0,100.0))
			sample.push_back(i);
	}
}

template<class COMP>
int GAEngine<COMP>::select_weighted(POPULATION& p)
{
	double sum=0.0;
	double zero_lim=0.000000000001;

	// total population fitness
	for(int i=0;i<p.size();i++)
	{
		sum+=(p[i].valid()?1.0/(p[i].fitness()?p[i].fitness():zero_lim):0.0);	//TODO sum can overflow?
	}
	// use a randomly selected threshold for a cumulative-sum selection
	double choice=sum*rnd_generate(0.0,1.0);
	for(int i=0;i<p.size();i++)
	{
		choice-=1.0/(p[i].fitness()?p[i].fitness():zero_lim);
		if(choice<=0.0)
			return i;
	}
	// choice is larger than total sum
	return p.size()-1;	// return the index to last (least-fit) genome
}
