#ifndef GA_ENGINE_H
#define GA_ENGINE_H
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <limits>
#include <functional>
#include <time.h>
#include <stdlib.h>
#ifdef SUPPORT_MPI
#include <mpi.h>
#endif
#include <limits>
#include "utils.h"
#include "virtexp.h"
#include "distributor.h"
#include <math.h>

extern int verbosity;

//Genome handler
class Genome
{
    private:
        ALLELE m_Alleles;
        double m_Fitness;
        bool m_Valid;

    public:
        Genome();
        Genome(const Genome& other);
        ~Genome();

		// assignment
        Genome operator=(const Genome& other);

		// Get an allele value from allele name (0.0 if not found)
        double allele(const std::wstring& name);

		// Get an allele value from indexing the genome (0.0 if out of range)
        double allele(int index);

		// Update allele in genome by name and value
        void allele(const std::wstring& name,double val);

		// Update the value of ith allele in genome
        void allele(int index,double val);

		// Get validity of genome
        bool valid() const { return m_Valid; }
        void valid(bool b) { m_Valid=b; }		// assign m_Valid member to b

		// Get the name of ith allele in genome (empty if oor)
        std::wstring name(int index);

		// Get the size of genome
        int size() { return m_Alleles.size(); }

		// Get the allele by indexing the genome sequence
        std::pair<std::wstring,double>& operator[](int index);

		// Get the fitness value
        double fitness() const { return m_Fitness; }
        void fitness(double v) { m_Fitness=v; m_Valid=(v!=INFINITY); }	// an INF fit genome is invalid
        
		// Compare genomes by fitness
		bool operator<(const Genome& other) const;
        bool operator>(const Genome& other) const;
        bool operator==(const Genome& other) const;

		// Test if two genomes are identical
        bool same(const Genome& other) const;

		// Store the genetic data in a temporary storage
        void var(VariablesHolder& v);

		// Rebuild a genome from a temporary sequence
        void set(VariablesHolder& v);
};

bool reverse_compare(const Genome& v1,const Genome& v2) { return (v1<v2); }
extern bool observer(WorkItem *w,double answer,void *);


template<typename COMP>
class GAEngine
{
    private:
        typedef std::vector<Genome> POPULATION;
        POPULATION m_Population;
        int m_MaxPopulation;
        std::vector<std::wstring> m_AlleleList;
        double m_CrossProbability;
        double m_MutationProbability;
        int m_crossPartition;
        int m_mutatePartition;
        int m_Generations;
        double m_bestFitness;
        bool m_bBestFitnessAssigned;
        VariablesHolder m_bestVariables;

    public:
        typedef Genome GENOME;

        GAEngine();
        ~GAEngine();

        double& prob_cross() { return m_CrossProbability; }
        double& prob_mutate() { return m_MutationProbability; }
        int& part_cross() { return m_crossPartition; }
        int& part_mutate() { return m_mutatePartition; }

		// Set the maximum population of GA
        void set_borders(int max_population);

		// Initialise the population with randomly generated genomes
        bool Initialise();

		// Size of a GAEngine
        int size() { return m_Population.size(); }

		// Add a new genotype into the central gene-pool
        void AddAllele(const std::wstring& name);

		// Set limits to an allele's values
        void AddLimit(const std::wstring& name,double lower,double upper);
		
		// Copy the gene-pool into a temporary storage
        void var_template(VariablesHolder& v);

		// Get the currently fittest genome and its fitness and store them
        double GetBest(VariablesHolder& v);

		// Create a work item to be processed from chromosome data
        WorkItem *var_to_workitem(VariablesHolder& h);

		// Process the results in the workitem by updating the fitness of the corresponding genome
        void process_workitem(WorkItem *w,double answer);

		// Run the GA engine for given number of generations
        void RunGenerations(int gener);

    private:
        typedef std::map<std::wstring,std::pair<double,double> > LIMITS;
        LIMITS m_Limits;

		// Print the genetic data of a member in population
		void print_genome(int ind_genome);

		// Print the genetic data of the current population
		void print_population();

		// Output a current summary of GA
		// TODO different verbosity settings
        void print_stage(int g);

		// mutate
		// if		name is non-emtpy wstring, only the allele with matching name is mutated at the mutation rate
		// else if	name is an empty wstring, mutate all alleles
		// setting mutate_all to false will mutate approx. just 1 allele
        void mutate(const std::wstring& name,Genome& g,bool mutate_all=false)
        {
            double prob=(mutate_all?101.0:100.0/g.size());

            for(int i=0;i<g.size();i++)
            {
                double p=rnd_generate(0.0,100.0);
                
				// mutate if ( p <= prob )
                if(p>prob)		// chance to skip mutation
                   continue;

                if(!name.size() || g.name(i)==name)		
                {
                    LIMITS::iterator it=m_Limits.find(g.name(i));	// check for param limits of this allele
                    double val;
                    if(it==m_Limits.end())
                    {
                        //no limits, just use [-RAND_MAX/2,RAND_MAX/2] as a limit
                        val=rnd_generate(-RAND_MAX*0.5,RAND_MAX*0.5);
                    }
                    else
                    {
						// restrict RNG to set limits
                        val=rnd_generate(it->second.first,it->second.second);
                    }
                    g.allele(i,val);	// set the RNG value to allele

                    if(name.size())		// only mutate this allele if name specified
                        break;
                }
            }
        }

        void mutate(double probability,Genome& g,int count=-1)
        {
            int cnt=0;
            double prob=rnd_generate(0.0,100.0);
 
            for(int i=0;i<g.size();i++)
            {
                if(prob<=probability)
                {
                    LIMITS::iterator it=m_Limits.find(g.name(i));
                    double val;
                    if(it==m_Limits.end())
                    {
                        //no limits, just use [-RAND_MAX/2,RAND_MAX] as a limit
                        val=rnd_generate(-RAND_MAX*0.5,RAND_MAX*0.5);
                    }
                    else
                    {
                        val=rnd_generate(it->second.first,it->second.second);
                    }
                    g.allele(i,val);
                    cnt++;
                    if(count>=0 && cnt>=count)
                       break;
                }
            }
        }

		// cross
        bool cross(Genome& one,Genome& two,int crosspoint,Genome& out)
        {
            if(one.size()!=two.size() || one.size()<crosspoint+1)
                return false;
            for(int i=0;i<crosspoint;i++)
                out[i]=one[i];
            for(int i=crosspoint;i<one.size();i++)
                out[i]=two[i];
            return true;
        }
        bool cross(Genome& one,Genome& two,int crosspoint)
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

		// build_rnd_sample
		// append a defined number of randomly selected indices to genomes onto sample
        void build_rnd_sample(std::vector<int>& sample,int count,bool reject_duplicates,bool check_valid)
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
					} while (check_valid && !m_Population[v].valid());
                } while(reject_duplicates && std::find(sample.begin(),sample.end(),v)!=sample.end());
                //Found next genome
                sample.push_back(v);
            }
        }
		
        void build_rnd_sample_tournament(std::vector<int>& sample,int count,bool reject_duplicates,bool check_valid)
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
                }
                while(reject_duplicates && std::find(sample.begin(),sample.end(),v)!=sample.end());
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

		// build_rnd_sample_rnd
        void build_rnd_sample_rnd(std::vector<int>& sample,double prob,bool check_valid)
        {
			// if check_valid, only appends indices of m_Population for which Genomes are valid, at given probability (%)
			// else (check_valid==false), appends Genomes at given rate (%)
            for(int i=0;i<m_Population.size();i++)
            {
                if((!check_valid || m_Population[i].valid()) && prob>=rnd_generate(0.0,100.0))
                    sample.push_back(i);
            }
        }

		// select_weighted
        int select_weighted(POPULATION& p)
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
};

#endif

