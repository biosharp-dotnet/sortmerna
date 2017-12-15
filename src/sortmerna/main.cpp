/**
 * @file main.cpp
 * @brief File containing the main function and argument parsing.
 * @parblock
 * SortMeRNA - next-generation reads filter for metatranscriptomic or total RNA
 * @copyright 2012-16 Bonsai Bioinformatics Research Group
 * @copyright 2014-16 Knight Lab, Department of Pediatrics, UCSD, La Jolla
 * @copyright 2016-17 Evguenia Kopylova
 *
 * SortMeRNA is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SortMeRNA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SortMeRNA.  If not, see <http://www.gnu.org/licenses/>.
 * @endparblock
 *
 * @contributors Jenya Kopylova, jenya.kopylov@gmail.com
 *               Laurent Noé, laurent.noe@lifl.fr
 *               Pierre Pericard, pierre.pericard@lifl.fr
 *               Daniel McDonald, wasade@gmail.com
 *               Mikaël Salson, mikael.salson@lifl.fr
 *               Hélène Touzet, helene.touzet@lifl.fr
 *               Rob Knight, robknight@ucsd.edu
 */

// TODO: BUG: if SMR headers moved down after 3rd party, 'timeval' struct gets 'redefined' - compiler error. That's a header mess bug.
// SMR
#include "options.hpp"
#include "paralleltraversal.hpp"
#include "common.hpp"
#include "output.hpp"

// standard
#include <limits>
#include <dirent.h>

#ifdef __APPLE__
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

// 3rd party
#include "zlib.h"


#if defined(_WIN32)
const char DELIM = ';';
#else
const char DELIM = ':';
#endif

using namespace std;

/*! @brief Measure time using this variable. */
timeval t;
double evalue = -1.0;
double align_id = -1.0;
double align_cov = -1.0;
int numcpu_gv = -1;
bool verbose = false;
bool pairedin_gv = false;
bool pairedout_gv = false;
bool logout_gv = false;
bool de_novo_otu_gv = false;
bool print_all_reads_gv = false;
long unsigned int pagesize_gv = sysconf(_SC_PAGE_SIZE);
long unsigned int maxpages_gv = 0;
long unsigned int map_size_gv = pagesize_gv;
bool map_size_set_gv = false;
bool samout_gv = false; // TODO: remove -> opts.samout
bool blastout_gv = false; // TODO: remove -> opts.blastout
vector<string> user_opts;
bool blast_tabular = false;
bool fastxout_gv = false;
bool otumapout_gv = false;
int32_t min_lis_gv = -1;
int32_t num_alignments_gv = -1; // default = 1 if '--num_alignments' not specified
int32_t seed_hits_gv = -1;
int32_t edges_gv = -1;
bool full_search_gv = false;
/*! @brief Version number */
char version_num[] = "2.1b, 03/03/2016";
bool as_percent_gv = false;
bool pid_gv = false;
int32_t num_best_hits_gv = 0;



/*! @fn welcome()
 *  @brief outputs copyright, disclaimer and contact information
 *  @param none
 #  @return none
 */
void welcome()
{
	printf("\n  Program:     SortMeRNA version %s\n", version_num);
	printf("  Copyright:    2012-17 Bonsai Bioinformatics Research Group:\n");
	printf("                LIFL, University Lille 1, CNRS UMR 8022, INRIA Nord-Europe\n");
	printf("                2014-17 Knight Lab:\n");
	printf("                Department of Pediatrics, UCSD, La Jolla\n");
	printf("  Disclaimer:   SortMeRNA comes with ABSOLUTELY NO WARRANTY; without even the\n");
	printf("                implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
	printf("                See the GNU Lesser General Public License for more details.\n");
	printf("  Contributors: Jenya Kopylova   jenya.kopylov@gmail.com \n");
	printf("                Laurent Noé      laurent.noe@lifl.fr\n");
	printf("                Pierre Pericard  pierre.pericard@lifl.fr\n");
	printf("                Daniel McDonald  wasade@gmail.com\n");
	printf("                Mikaël Salson    mikael.salson@lifl.fr\n");
	printf("                Hélène Touzet    helene.touzet@lifl.fr\n");
	printf("                Rob Knight       robknight@ucsd.edu\n\n");
}



/*! @fn printlist()
 *  @brief outputs options menu
 *  @param none
 *  @return none
 */
void printlist()
{
	printf("\n  usage:   ./sortmerna --ref db.fasta,db.idx --reads file.fa --aligned base_name_output [OPTIONS]:\n");
#ifdef HAVE_LIBZ
	printf("  OR\n");
	printf("  usage:   ./sortmerna --ref db.fasta,db.idx --reads-gz file.fa.gz --aligned base_name_output [OPTIONS]:\n");
#endif
	printf("\n  -------------------------------------------------------------------------------------------------------------\n");
	printf("  | parameter          value           description                                                    default |\n");
	printf("  -------------------------------------------------------------------------------------------------------------\n");
	printf("     %s--ref%s             %sSTRING,STRING%s   FASTA reference file, index file                               %smandatory%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[0;32m", "\033[0m");
	printf("                                         (ex. --ref /path/to/file1.fasta,/path/to/index1)\n");
	printf("                                         If passing multiple reference files, separate \n");
	printf("                                         them using the delimiter ':',\n");
	printf("                                         (ex. --ref /path/to/file1.fasta,/path/to/index1:/path/to/file2.fasta,path/to/index2)\n");
	printf("     %s--reads%s           %sSTRING%s          FASTA/FASTQ raw reads file                                     %smandatory%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[0;32m", "\033[0m");
#ifdef HAVE_LIBZ
	printf("       OR\n");
	printf("     %s--reads-gz%s        %sSTRING%s          FASTA/FASTQ compressed (with gzip) reads file                  %smandatory%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[0;32m", "\033[0m");
#endif
	printf("     %s--aligned%s         %sSTRING%s          aligned reads filepath + base file name                        %smandatory%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[0;32m", "\033[0m");
	printf("                                         (appropriate extension will be added)\n\n");
	printf("   [COMMON OPTIONS]: \n");
	printf("     %s--other%s           %sSTRING%s          rejected reads filepath + base file name\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m");
	printf("                                         (appropriate extension will be added)\n");
	printf("     %s--fastx%s           %sBOOL%s            output FASTA/FASTQ file                                        %soff%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("                                         (for aligned and/or rejected reads)\n");
	printf("     %s--sam%s             %sBOOL%s            output SAM alignment                                           %soff%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("                                         (for aligned reads only)\n");
	printf("     %s--SQ%s              %sBOOL%s            add SQ tags to the SAM file                                    %soff%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("     %s--blast%s           %sSTRING%s          output alignments in various Blast-like formats                \n", "\033[1m", "\033[0m", "\033[4m", "\033[0m");
	printf("                                        '0' - pairwise\n");
	printf("                                        '1' - tabular (Blast -m 8 format)\n");
	printf("                                        '1 cigar' - tabular + column for CIGAR \n");
	printf("                                        '1 cigar qcov' - tabular + columns for CIGAR\n");
	printf("                                                         and query coverage\n");
	printf("                                        '1 cigar qcov qstrand' - tabular + columns for CIGAR,\n");
	printf("                                                                query coverage and strand\n");
	printf("     %s--log%s             %sBOOL%s            output overall statistics                                      %soff%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
#ifdef NOMASK_option
	printf("     %s--no-mask%s         %sBOOL%s            do not mask low occurrence (L/2)-mers when searching           %son%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("                                       for seeds of length L\n");
#endif
	printf("     %s--num_alignments%s  %sINT%s             report first INT alignments per read reaching E-value          %s-1%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("                                        (--num_alignments 0 signifies all alignments will be output)\n");
	printf("       %sor%s (default)\n", "\033[31m", "\033[0m");
	printf("     %s--best%s            %sINT%s             report INT best alignments per read reaching E-value           %s1%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("                                         by searching --min_lis INT candidate alignments\n");
	printf("                                        (--best 0 signifies all candidate alignments will be searched)\n");
	printf("     %s--min_lis%s         %sINT%s             search all alignments having the first INT longest LIS         %s2%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("                                         LIS stands for Longest Increasing Subsequence, it is \n");
	printf("                                         computed using seeds' positions to expand hits into\n");
	printf("                                         longer matches prior to Smith-Waterman alignment. \n");
	printf("     %s--print_all_reads%s %sBOOL%s            output null alignment strings for non-aligned reads            %soff%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("                                         to SAM and/or BLAST tabular files\n");
	printf("     %s--paired_in%s       %sBOOL%s            both paired-end reads go in --aligned fasta/q file             %soff%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("                                        (interleaved reads only, see Section 4.2.4 of User Manual)\n");
	printf("     %s--paired_out%s      %sBOOL%s            both paired-end reads go in --other fasta/q file               %soff%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");\
		printf("                                        (interleaved reads only, see Section 4.2.4 of User Manual)\n");
	printf("     %s--match %s          %sINT%s             SW score (positive integer) for a match                        %s2%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("     %s--mismatch%s        %sINT%s             SW penalty (negative integer) for a mismatch                   %s-3%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("     %s--gap_open%s        %sINT%s             SW penalty (positive integer) for introducing a gap            %s5%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("     %s--gap_ext%s         %sINT%s             SW penalty (positive integer) for extending a gap              %s2%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("     %s-N%s                %sINT%s             SW penalty for ambiguous letters (N's)                         %sscored as --mismatch%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("     %s-F%s                %sBOOL%s            search only the forward strand                                 %soff%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("     %s-R%s                %sBOOL%s            search only the reverse-complementary strand                   %soff%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("     %s-a%s                %sINT%s             number of threads to use                                       %s1%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("     %s-e%s                %sDOUBLE%s          E-value threshold                                              %s1%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	// RAM cannot support 1GB default
	if (1073741824 / pagesize_gv > maxpages_gv / 2)
		printf("     %s-m%s                %sINT%s             INT Mbytes for loading reads in memory with mmap             %s%lu%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", ((pagesize_gv*(maxpages_gv / 2)) / 1048576), "\033[0m");
	// RAM can support at least 1GB default
	else
		printf("     %s-m%s                %sINT%s             INT Mbytes for loading reads in memory with mmap               %s[suggested: 1024]%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("                                        (maximum -m INT is %lu)\n", (((maxpages_gv / 2)*pagesize_gv) / 1048576));
	printf("                                        (NOTE: If this option is chosen, reads will be loaded using\n");
	printf("                                         mmap rather than the default generic stream buffer; if the\n");
	printf("                                         input reads are in compressed format, this option will be\n");
	printf("                                         ignored; loading reads with mmap can be faster and is\n");
	printf("                                         recommended for large files which cannot fit into\n");
	printf("                                         available RAM)\n");
	printf("     %s-v%s                %sBOOL%s            verbose                                                        %soff%s\n\n\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("   [OTU PICKING OPTIONS]: \n");
	printf("     %s--id%s              %sDOUBLE%s          %%id similarity threshold (the alignment must                   %s0.97%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("                                         still pass the E-value threshold)\n");
	printf("     %s--coverage%s        %sDOUBLE%s          %%query coverage threshold (the alignment must                  %s0.97%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("                                         still pass the E-value threshold)\n");
	printf("     %s--de_novo_otu%s     %sBOOL%s            FASTA/FASTQ file for reads matching database < %%id             %soff%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("                                         (set using --id) and < %%cov (set using --coverage) \n");
	printf("                                         (alignment must still pass the E-value threshold)\n");
	printf("     %s--otu_map%s         %sBOOL%s            output OTU map (input to QIIME's make_otu_table.py)            %soff%s\n\n\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("   [ADVANCED OPTIONS] (see SortMeRNA user manual for more details): \n");
	printf("    %s--passes%s           %sINT,INT,INT%s     three intervals at which to place the seed on the read         %sL,L/2,3%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("                                         (L is the seed length set in ./indexdb_rna)\n");
	printf("    %s--edges%s            %sINT%s             number (or percent if INT followed by %% sign) of               %s4%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("                                         nucleotides to add to each edge of the read\n");
	printf("                                         prior to SW local alignment \n");
	printf("    %s--num_seeds%s        %sINT%s             number of seeds matched before searching                       %s2%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("                                         for candidate LIS \n");
	printf("    %s--full_search%s      %sBOOL%s            search for all 0-error and 1-error seed                        %soff%s\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("                                         matches in the index rather than stopping\n");
	printf("                                         after finding a 0-error match (<1%% gain in\n");
	printf("                                         sensitivity with up four-fold decrease in speed)\n");
	printf("    %s--pid%s              %sBOOL%s            add pid to output file names                                   %soff%s\n\n\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m", "\033[4m", "\033[0m");
	printf("   [HELP]:\n");
	printf("     %s-h%s                %sBOOL%s            help\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m");
	printf("     %s--version%s         %sBOOL%s            SortMeRNA version number\n\n\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m");
	printf("     %s--report%s          %sINT%s     Report generation: 0 (no reports - only align), 1 (only report), 2 (both align and report)\n\n\n", "\033[1m", "\033[0m", "\033[4m", "\033[0m");
	exit(EXIT_FAILURE);
}//~printlist()

void Runopts::optReads(char **argv, int &narg)
{
	if (have_reads_gz)
	{
		fprintf(stderr, "\n %sERROR%s: option --reads-gz has also been set, only one of "
			"--reads-gz or --reads is permitted\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	if (argv[narg + 1] == NULL)
	{
		fprintf(stderr, "\n  %sERROR%s: a path to a reads FASTA/FASTQ file "
			"must be given after the option --reads\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	else
	{
		// check the file exists
		if (FILE *file = fopen(argv[narg + 1], "rb"))
		{
			// get size of file
			fseek(file, 0, SEEK_END);
			size_t filesize = ftell(file);

			// set exit BOOL to exit program after outputting
			// empty files, sortmerna will not execute after
			// that call (in paralleltraversal.cpp)
			if (!filesize) exit_early = true;
			// reset file pointer to start of file
			fseek(file, 0, SEEK_SET);

			readsfile = argv[narg + 1];
			narg += 2;
			fclose(file);

			have_reads = true;
		}
		else
		{
			fprintf(stderr, "\n  %sERROR%s: the file %s could not be opened: "
				"%s.\n\n", startColor, "\033[0m", argv[narg + 1], strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
} // ~Runopts::optReads

void Runopts::optReadsGz(char **argv, int &narg)
{
	if (have_reads)
	{
		fprintf(stderr, "\n %sERROR%s: option --reads has also been set, only one of "
			"--reads or --reads-gz is permitted\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	if (argv[narg + 1] == NULL)
	{
		fprintf(stderr, "\n  %sERROR%s: a path to a reads FASTA/FASTQ compressed (.zip, .gz) file "
			"must be given after the option --reads-gz\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	else
	{
		// check the file exists
		if (gzFile file = gzopen(argv[narg + 1], "r"))
		{
			// get size of file
			gzseek(file, 0, SEEK_END);
			size_t filesize = gztell(file);

			// set exit BOOL to exit program after outputting
			// empty files, sortmerna will not execute after
			// that call (in paralleltraversal.cpp)
			if (!filesize) exit_early = true;
			// reset file pointer to start of file
			gzseek(file, 0, SEEK_SET);

			readsfile = argv[narg + 1];
			narg += 2;
			gzclose(file);

			have_reads_gz = true;
		}
		else
		{
			fprintf(stderr, "\n  %sERROR%s: the file %s could not be opened: "
				"%s.\n\n", startColor, "\033[0m", argv[narg + 1], strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
} // ~Runopts::optReadsGz

void Runopts::optRef(char **argv, int &narg)
{
	if (argv[narg + 1] == NULL)
	{
		fprintf(stderr, "\n  %sERROR%s: --ref must be followed by at least one entry "
			"(ex. --ref /path/to/file1.fasta,/path/to/index1)\n\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	// path exists, check path
	else
	{
		char *ptr = argv[narg + 1];
		while (*ptr != '\0')
		{
			// get the FASTA file path + name
			char fastafile[2000];
			char *ptr_fastafile = fastafile;

			// the reference database FASTA file
			while (*ptr != ',' && *ptr != '\0')
			{
				*ptr_fastafile++ = *ptr++;
			}
			*ptr_fastafile = '\0';
			if (*ptr == '\0')
			{
				fprintf(stderr, "   %sERROR%s: the FASTA reference file name %s must be followed "
					" by an index name.\n\n", startColor, "\033[0m", fastafile);
				exit(EXIT_FAILURE);
			}
			ptr++; //skip the ',' delimiter

				   // check reference FASTA file exists & is not empty
			if (FILE *file = fopen(fastafile, "rb"))
			{
				// get file size
				fseek(file, 0, SEEK_END);
				size_t filesize = ftell(file);
				if (!filesize) exit_early = true;
				// reset file pointer to start of file
				fseek(file, 0, SEEK_SET);
				fclose(file);
			}
			else
			{
				fprintf(stderr, "\n  %sERROR%s: the file %s could not be opened: "
					" %s.\n\n", startColor, "\033[0m", fastafile, strerror(errno));
				exit(EXIT_FAILURE);
			}

			// get the index path + name
			char indexfile[2000];
			char *ptr_indexfile = indexfile;
			// the reference database index name
			while (*ptr != DELIM && *ptr != '\0') *ptr_indexfile++ = *ptr++;
			*ptr_indexfile = '\0';
			if (*ptr != '\0') ptr++; //skip the ':' delimiter

									 // check the directory where to write the index exists
			char dir[500];
			char *ptr_end = strrchr(indexfile, '/');
			if (ptr_end != NULL)
			{
				memcpy(dir, indexfile, (ptr_end - indexfile));
				dir[(int)(ptr_end - indexfile)] = '\0';
			}
			else
			{
				strcpy(dir, "./");
			}

			if (DIR *dir_p = opendir(dir)) closedir(dir_p);
			else
			{
				if (ptr_end != NULL)
					fprintf(stderr, "\n  %sERROR%s: the directory %s for writing index "
						"'%s' could not be opened. The full directory path must be "
						"provided (ex. no '~'). \n\n", startColor, "\033[0m",
						dir, ptr_end + 1);
				else
					fprintf(stderr, "\n  %sERROR%s: the directory %s for writing index "
						"'%s' could not be opened. The full directory path must be "
						"provided (ex. no '~'). \n\n", startColor, "\033[0m",
						dir, indexfile);

				exit(EXIT_FAILURE);
			}

			// check index file names are distinct
			for (int i = 0; i < (int)indexfiles.size(); i++)
			{
				if ((indexfiles[i].first).compare(fastafile) == 0)
				{
					fprintf(stderr, "\n  %sWARNING%s: the FASTA file %s has been entered "
						"twice in the list. It will be searched twice. "
						"\n\n", "\033[0;33m", "\033[0m", fastafile);
				}
				else if ((indexfiles[i].second).compare(indexfile) == 0)
				{
					fprintf(stderr, "\n  %sWARNING%s: the index name %s has been entered "
						"twice in the list. It will be searched twice.\n\n", "\033[0;33m",
						"\033[0m", indexfile);
				}
			}

			indexfiles.push_back(pair<string, string>(fastafile, indexfile));

		}//~while (*ptr != '\0')

		narg += 2;

	}//~else
} // ~Runopts::optRef

void Runopts::optAligned(char **argv, int &narg)
{
	if ((argv[narg + 1] == NULL) || (argv[narg + 1][0] == '-'))
	{
		fprintf(stderr, "\n  %sERROR%s: a filename must follow the option --aligned "
			"[STRING]\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	else
	{
		// check if the directory where to write exists
		char dir[500];
		char *ptr = strrchr(argv[narg + 1], '/');
		if (ptr != NULL)
		{
			memcpy(dir, argv[narg + 1], (ptr - argv[narg + 1]));
			dir[(int)(ptr - argv[narg + 1])] = '\0';
		}
		else
		{
			strcpy(dir, "./");
		}

		if (DIR *dir_p = opendir(dir))
		{
			ptr_filetype_ar = argv[narg + 1];
			narg += 2;
			closedir(dir_p);
		}
		else
		{
			fprintf(stderr, "\n  %sERROR%s: the --aligned [STRING] directory "
				"%s could not be opened: %s.\n\n", startColor, "\033[0m",
				dir, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
} // ~Runopts::optAligned

void Runopts::optOther(char **argv, int &narg)
{
	if ((argv[narg + 1] == NULL) || (argv[narg + 1][0] == '-'))
	{
		fprintf(stderr, "\n  %sERROR%s: a filename must follow the option "
			"--other [STRING]\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	else
	{
		// check if the directory where to write exists
		char dir[500];
		char *ptr = strrchr(argv[narg + 1], '/');
		if (ptr != NULL)
		{
			memcpy(dir, argv[narg + 1], (ptr - argv[narg + 1]));
			dir[(int)(ptr - argv[narg + 1])] = '\0';
		}
		else
		{
			strcpy(dir, "./");
		}

		if (DIR *dir_p = opendir(dir))
		{
			ptr_filetype_or = argv[narg + 1];
			narg += 2;
			closedir(dir_p);
		}
		else
		{
			fprintf(stderr, "\n  %sERROR%s: the --other %s directory could not be "
				"opened, please check it exists.\n\n", startColor,
				"\033[0m", dir);
			exit(EXIT_FAILURE);
		}
	}
} // ~Runopts::optOther

void Runopts::optLog(char **argv, int &narg)
{
	if (logout_gv)
	{
		fprintf(stderr, "\n  %sERROR%s: --log has already been set once.\n",
			startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	else
	{
		logout_gv = true;
		narg++;
	}
} // ~Runopts::optLog

void Runopts::optDeNovoOtu(char **argv, int &narg)
{
	if (de_novo_otu_gv)
	{
		fprintf(stderr, "\n  %sERROR%s: --de_novo_otu has already been set once.\n",
			startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	else
	{
		de_novo_otu_gv = true;
		narg++;
	}
} // ~Runopts::optDeNovoOtu

void Runopts::optOtuMap(char **argv, int &narg)
{
	if (otumapout_gv)
	{
		fprintf(stderr, "\n  %sERROR%s: --otu_map has already been set once.\n",
			startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	else
	{
		otumapout_gv = true;
		narg++;
	}
} // ~Runopts::optOtuMap

void Runopts::optPrintAllReads(char **argv, int &narg)
{
	if (print_all_reads_gv)
	{
		fprintf(stderr, "\n  %sERROR%s: --print_all_reads has already been set once.\n",
			startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	else
	{
		print_all_reads_gv = true;
		narg++;
	}
} // ~Runopts::optPrintAllReads

void Runopts::optPid(char **argv, int &narg)
{
	if (pid_gv)
	{
		fprintf(stderr, "\n  %sERROR%s: --pid has already been set once.\n",
			startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	else
	{
		pid_gv = true;
		narg++;
	}
} // ~Runopts::optPid

void Runopts::optPairedIn(char **argv, int &narg)
{
	if (pairedin_gv)
	{
		fprintf(stderr, "\n  %sERROR%s: --paired_in has already been set once.\n",
			startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	else if (pairedout_gv)
	{
		fprintf(stderr, "\n  %sERROR%s: --paired_out has been set, please choose "
			"one or the other, or use the default option.\n",
			startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	else
	{
		pairedin_gv = true;
		narg++;
	}
} // ~Runopts::optPairedIn

void Runopts::optPairedOut(char **argv, int &narg)
{
	if (pairedout_gv)
	{
		fprintf(stderr, "\n  %sERROR%s: --paired_out has already been set once.\n",
			startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	else if (pairedin_gv)
	{
		fprintf(stderr, "\n %sERROR%s: --paired_in has been set, please choose one "
			"or the other, or use the default option.\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	else
	{
		pairedout_gv = true;
		narg++;
	}
} // ~Runopts::optPairedOut

void Runopts::optMatch(char **argv, int &narg)
{
	if (argv[narg + 1] == NULL)
	{
		fprintf(stderr, "\n  %sERROR%s: --match [INT] requires a positive integer as "
			"input (ex. --match 2).\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	// set match
	if (!match_set)
	{
		match = atoi(argv[narg + 1]);
		narg += 2;
		match_set = true;
	}
	else
	{
		fprintf(stderr, "\n  %sERROR%s: --match [INT] has been set twice, please "
			"verify your choice\n\n", startColor, "\033[0m");
		printlist();
	}
} // ~Runopts::optMatch

void Runopts::optMismatch(char **argv, int &narg)
{
	if (argv[narg + 1] == NULL)
	{
		fprintf(stderr, "\n  %sERROR%s: --mismatch [INT] requires a negative integer "
			"input (ex. --mismatch -2)\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	// set mismatch
	if (!mismatch_set)
	{
		mismatch = atoi(argv[narg + 1]);
		if (mismatch > 0)
		{
			fprintf(stderr, "\n  %sERROR%s: --mismatch [INT] requires a negative "
				"integer input (ex. --mismatch -2)\n", startColor, "\033[0m");
			exit(EXIT_FAILURE);
		}
		narg += 2;
		mismatch_set = true;
	}
	else
	{
		printf("\n  %sERROR%s: --mismatch [INT] has been set twice, please verify "
			"your choice\n\n", startColor, "\033[0m");
		printlist();
	}
} // ~Runopts::optMismatch

void Runopts::optGapOpen(char **argv, int &narg)
{
	if (argv[narg + 1] == NULL)
	{
		fprintf(stderr, "\n  %sERROR%s: --gap_open [INT] requires a positive integer "
			"as input (ex. --gap_open 5)\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	// set gap open
	if (!gap_open_set)
	{
		gap_open = atoi(argv[narg + 1]);
		if (gap_open < 0)
		{
			fprintf(stderr, "\n  %sERROR%s: --gap_open [INT] requires a positive "
				"integer as input (ex. --gap_open 5)\n", startColor, "\033[0m");
			exit(EXIT_FAILURE);
		}
		narg += 2;
		gap_open_set = true;
	}
	else
	{
		printf("\n  %sERROR%s: --gap_open [INT] has been set twice, please verify "
			"your choice\n\n", startColor, "\033[0m");
		printlist();
	}
} // ~Runopts::optGapOpen

void Runopts::optGapExt(char **argv, int &narg)
{
	if (argv[narg + 1] == NULL)
	{
		fprintf(stderr, "\n  %sERROR%s: --gap_ext [INT] requires a positive integer "
			"as input (ex. --gap_ext 2)\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	// set gap extend
	if (!gap_ext_set)
	{
		gap_extension = atoi(argv[narg + 1]);
		if (gap_extension < 0)
		{
			fprintf(stderr, "\n  %sERROR%s: --gap_ext [INT] requires a positive "
				"integer as input (ex. --gap_ext 2)\n", startColor, "\033[0m");
			exit(EXIT_FAILURE);
		}
		narg += 2;
		gap_ext_set = true;
	}
	else
	{
		fprintf(stderr, "\n  %sERROR%s: --gap_ext [INT] has been set twice, please "
			"verify your choice\n\n", startColor, "\033[0m");
		printlist();
	}
} // ~Runopts::optGapExt

void Runopts::optNumSeeds(char **argv, int &narg)
{
	if (argv[narg + 1] == NULL)
	{
		fprintf(stderr, "\n  %sERROR%s: --num_seeds [INT] requires a positive integer "
			"as input (ex. --num_seeds 6)\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	// set number of seeds
	if (seed_hits_gv < 0)
	{
		char* end = 0;
		seed_hits_gv = (int)strtol(argv[narg + 1], &end, 10); // convert to integer
		if (seed_hits_gv <= 0)
		{
			fprintf(stderr, "\n  %sERROR%s: --num_seeds [INT] requires a positive "
				"integer (>0) as input (ex. --num_seeds 6)\n", startColor, "\033[0m");
			exit(EXIT_FAILURE);
		}
		narg += 2;
	}
	else
	{
		fprintf(stderr, "\n  %sERROR%s: --num_seeds [INT] has been set twice, please "
			"verify your choice\n\n", startColor, "\033[0m");
		printlist();
	}
} // ~Runopts::optNumSeeds

void Runopts::optFastx(char **argv, int &narg)
{
	if (fastxout_gv)
	{
		fprintf(stderr, "\n  %sERROR%s: --fastx has already been set once.\n\n",
			startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	else
	{
		fastxout_gv = true;
		narg++;
	}
} // ~Runopts::optFastx

void Runopts::optSam(char **argv, int &narg)
{
	if (samout)
	{
		fprintf(stderr, "\n  %sERROR%s: --sam has already been set once.\n\n",
			startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	else
	{
		samout = true;
		narg++;
	}
} // ~Runopts::optSam

void Runopts::optBlast(char **argv, int &narg)
{
	if (blastout)
	{
		fprintf(stderr, "\n  %sERROR%s: --blast [STRING] has already been set once.\n\n",
			startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	else
	{
		string str(argv[narg + 1]);
		// split blast options into vector by space
		istringstream iss(str);
		do
		{
			string s;
			iss >> s;
			user_opts.push_back(s);
		} while (iss);
		// remove the end of file entry
		user_opts.pop_back();
		bool blast_human_readable = false;
		vector<string> supported_opts;
		supported_opts.push_back("0");
		supported_opts.push_back("1");
		supported_opts.push_back("cigar");
		supported_opts.push_back("qstrand");
		supported_opts.push_back("qcov");
		// check user options are supported
		for (uint32_t i = 0; i < user_opts.size(); i++)
		{
			bool match_found = false;
			string opt = user_opts[i];
			vector<string>::iterator it;
			for (it = supported_opts.begin(); it != supported_opts.end(); ++it)
			{
				if (opt.compare(*it) == 0)
				{
					if (opt.compare("0") == 0) blast_human_readable = true;
					else if (opt.compare("1") == 0) {
						blast_tabular = true; // TODO: remove
						blastFormat = BlastFormat::TABULAR;
					}
					match_found = true;
					break;
				}
			}
			if (!match_found)
			{
				fprintf(stderr, "\n  %sERROR%s: `%s` is not supported in --blast [STRING].\n\n",
					startColor, "\033[0m", opt.c_str());
				exit(EXIT_FAILURE);
			}
		}
		// more than 1 field with blast human-readable format given
		if (blast_human_readable && (user_opts.size() > 1))
		{
			fprintf(stderr, "\n  %sERROR%s: for human-readable format, --blast [STRING] cannot contain "
				"more fields than '0'.\n\n", startColor, "\033[0m");
			exit(EXIT_FAILURE);
		}
		// both human-readable and tabular format options have been chosen
		if (blast_human_readable && blast_tabular)
		{
			fprintf(stderr, "\n  %sERROR%s: --blast [STRING] can only have one of the options "
				"'0' (human-readable) or '1' (tabular).\n\n", startColor, "\033[0m");
			exit(EXIT_FAILURE);
		}
		blastout = true;
		narg += 2;
	}
} // ~Runopts::optBlast

void Runopts::optMinLis(char **argv, int &narg)
{
	if (argv[narg + 1] == NULL)
	{
		fprintf(stderr, "\n  %sERROR%s: --min_lis [INT] requires an integer (>=0) as "
			"input (ex. --min_lis 2) (note: 0 signifies to search all high scoring "
			"reference sequences).\n\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	// min_lis_gv has already been set
	else if (min_lis_gv_set)
	{
		fprintf(stderr, "\n  %sERROR%s: --min_lis [INT] has been set twice, please "
			"verify your choice.\n\n", startColor, "\033[0m");
		printlist();
	}
	else
	{
		if ((sscanf(argv[narg + 1], "%d", &min_lis_gv) != 1) || (min_lis_gv < 0))
		{
			fprintf(stderr, "\n  %sERROR%s: --min_lis [INT] must be >= 0 (0 signifies "
				"to search all high scoring reference sequences).\n\n",
				startColor, "\033[0m");
			exit(EXIT_FAILURE);
		}
		narg += 2;
		min_lis_gv_set = true;
	}
} // ~Runopts::optMinLis

void Runopts::optBest(char **argv, int &narg)
{
	if (argv[narg + 1] == NULL)
	{
		fprintf(stderr, "\n  %sERROR%s: --best [INT] requires an integer (> 0) "
			"as input (ex. --best 2).\n\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	// best_gv_set has already been set
	else if (best_gv_set)
	{
		fprintf(stderr, "\n  %sERROR%s: --best [INT] has been set twice, please "
			"verify your choice.\n\n", startColor, "\033[0m");
		printlist();
	}
	else
	{
		if ((sscanf(argv[narg + 1], "%d", &num_best_hits_gv) != 1))
		{
			fprintf(stderr, "\n  %sERROR%s: could not read --best [INT] as integer.\n\n",
				startColor, "\033[0m");
			exit(EXIT_FAILURE);
		}
		narg += 2;
		best_gv_set = true;
	}
} // ~Runopts::optBest

void Runopts::optNumAlignments(char **argv, int &narg)
{
	if (argv[narg + 1] == NULL)
	{
		fprintf(stderr, "\n  %sERROR%s: --num_alignments [INT] requires an integer "
			"(>=0) as input (ex. --num_alignments 2) (note: 0 signifies to "
			"output all alignments).\n\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	// --num_alignments [INT] has already been set
	else if (num_alignments_gv_set)
	{
		fprintf(stderr, "\n  %sERROR%s:--num_alignments [INT] has been set twice, "
			"please verify your command parameters.\n\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	// set number of alignments to output reaching the E-value
	else
	{
		num_alignments_gv = atoi(argv[narg + 1]);
		if (num_alignments_gv < 0)
		{
			fprintf(stderr, "\n  %sERROR%s: --num_alignments [INT] must be >= 0 "
				"(0 signifies to output all alignments).\n\n",
				startColor, "\033[0m");
			exit(EXIT_FAILURE);
		}
		narg += 2;
		num_alignments_gv_set = true;
	}
} // ~Runopts::optNumAlignments

void Runopts::optEdges(char **argv, int &narg)
{
	// --edges is already set
	if (edges_set)
	{
		fprintf(stderr, "\n  %sERROR%s: --edges [INT]%% has already been set once.\n\n",
			startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	else
	{
		char *end = 0;
		// find if % sign exists
		char* test = strpbrk(argv[narg + 1], "%");
		if (test != NULL)
			as_percent_gv = true;
		// convert to integer
		edges_gv = (int)strtol(argv[narg + 1], &end, 10);

		if (edges_gv < 1 || edges_gv > 10)
		{
			fprintf(stderr, "\n  %sERROR%s: --edges [INT]%% requires a positive integer "
				"between 0-10 as input (ex. --edges 4).\n", startColor, "\033[0m");
			exit(EXIT_FAILURE);
		}

		narg += 2;
	}
} // ~Runopts::optEdges

void Runopts::optFullSearch(char **argv, int &narg)
{
	if (full_search_set)
	{
		fprintf(stderr, "\n  %sERROR%s: BOOL --full_search has been set twice, please "
			"verify your choice.\n\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	else
	{
		full_search_set = true;
		full_search_gv = true;
		narg++;
	}
} // ~Runopts::optFullSearch

void Runopts::optSQ(char **argv, int &narg)
{
	if (yes_SQ)
	{
		fprintf(stderr, "\n  %sERROR%s: BOOL --SQ has been set twice, please verify "
			"your choice.\n\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	else
	{
		yes_SQ = true;
		narg++;
	}
} // ~Runopts::optSQ

void Runopts::optPasses(char **argv, int &narg)
{
	if (passes_set)
	{
		fprintf(stderr, "\n  %sERROR%s: --passes [INT,INT,INT] has been set twice, "
			"please verify your choice.\n\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	// set passes
	else
	{
		vector<uint32_t> skiplengths_v;
		char *end = 0;
		int32_t t = (int)strtol(strtok(argv[narg + 1], ","), &end, 10);
		if (t > 0) skiplengths_v.push_back(t);
		else
		{
			fprintf(stderr, "\n  %sERROR%s: all three integers in --passes [INT,INT,INT] "
				"must contain positive integers where 0<INT<(shortest read length)."
				"\n\n", startColor, "\033[0m");
			exit(EXIT_FAILURE);
		}
		t = (int)strtol(strtok(NULL, ","), &end, 10);
		if (t > 0) skiplengths_v.push_back(t);
		else
		{
			fprintf(stderr, "\n  %sERROR%s: all three integers in --passes [INT,INT,INT] "
				"must contain positive integers where 0<INT<(shortest read length). "
				"\n\n", startColor, "\033[0m");
			exit(EXIT_FAILURE);
		}
		t = (int)strtol(strtok(NULL, ","), &end, 10);
		if (t > 0) skiplengths_v.push_back(t);
		else
		{
			fprintf(stderr, "\n  %sERROR%s: all three integers in --passes [INT,INT,INT] "
				"must contain positive integers where 0<INT<(shortest read length)."
				"\n\n", startColor, "\033[0m");
			exit(EXIT_FAILURE);
		}

		skiplengths.push_back(skiplengths_v);
		narg += 2;
		passes_set = true;
	}
} // ~Runopts::optPasses

void Runopts::optId(char **argv, int &narg)
{
	// % id
	if (align_id < 0)
	{
		if ((sscanf(argv[narg + 1], "%lf", &align_id) != 1) ||
			(align_id < 0) || (align_id > 1))
		{
			fprintf(stderr, "\n  %sERROR%s: --id [DOUBLE] must be a positive float "
				"with value 0<=id<=1.\n\n", startColor, "\033[0m");
			exit(EXIT_FAILURE);
		}
		narg += 2;
	}
	else
	{
		fprintf(stderr, "\n  %sERROR%s: --id [DOUBLE] has been set twice, please "
			"verify your command parameters.\n\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
} // ~Runopts::optId

void Runopts::optCoverage(char **argv, int &narg)
{
	// % query coverage
	if (align_cov < 0)
	{
		if ((sscanf(argv[narg + 1], "%lf", &align_cov) != 1) ||
			(align_cov < 0) || (align_cov > 1))
		{
			fprintf(stderr, "\n  %sERROR%s: --coverage [DOUBLE] must be a positive "
				"float with value 0<=id<=1.\n\n", startColor, "\033[0m");
			exit(EXIT_FAILURE);
		}
		narg += 2;
	}
	else
	{
		fprintf(stderr, "\n  %sERROR%s: --coverage [DOUBLE] has been set twice, please "
			"verify your command parameters.\n\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
} // ~Runopts::optCoverage

void Runopts::optVersion(char **argv, int &narg)
{
	fprintf(stderr, "\n  SortMeRNA version %s\n\n", version_num);
	exit(EXIT_SUCCESS);
} // ~Runopts::optVersion

void Runopts::optUnknown(char **argv, int &narg, char * opt)
{
	fprintf(stderr, "\n  %sERROR%s: option --%s is not an option.\n\n",
		startColor, "\033[0m", opt);
	printlist();
} // ~Runopts::optUnknown

void Runopts::opt_a_NumCpus(char **argv, int &narg)
{
	// the number of cpus has been set twice
	if (numcpu_gv == -1)
	{
		numcpu_gv = atof(argv[narg + 1]);
		narg += 2;
	}
	else
	{
		printf("\n  %sERROR%s: -a [INT] has been set twice, please verify "
			"your command parameters.\n\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
} // ~Runopts::opt_a_NumCpus

void Runopts::opt_e_Evalue(char **argv, int &narg)
{
	// E-value
	if (argv[narg + 1] == NULL)
	{
		fprintf(stderr, "\n  %sERROR%s: -e [DOUBLE] requires a positive double "
			"as input (ex. --e 1e-5)\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	if (evalue < 0)
	{
		sscanf(argv[narg + 1], "%lf", &evalue);
		if (evalue < 0)
		{
			fprintf(stderr, "\n  %sERROR%s: -e [DOUBLE] requires a positive double "
				"as input (ex. --e 1e-5)\n", startColor, "\033[0m");
			exit(EXIT_FAILURE);
		}
		narg += 2;
	}
	else
	{
		fprintf(stderr, "\n  %sERROR%s: -e [DOUBLE] has been set twice, please verify "
			"your command parameters.\n\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
} // ~Runopts::opt_e_Evalue

void Runopts::opt_F_ForwardOnly(char **argv, int &narg)
{
	// only forward strand
	if (!forward)
	{
		forward = true;
		narg++;
	}
	else
	{
		fprintf(stderr, "\n  %sERROR%s: BOOL -F has been set more than once, please check "
			"your command parameters.\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
} // ~Runopts::opt_F_ForwardOnly

void Runopts::opt_R_ReverseOnly(char **argv, int &narg)
{
	// only reverse strand
	if (!reverse)
	{
		reverse = true;
		narg++;
	}
	else
	{
		fprintf(stderr, "\n  %sERROR%s: BOOL -R has been set more than once, please check "
			"your command parameters.\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
} // ~Runopts::opt_R_ReverseOnly

void Runopts::opt_h_Help()
{
	welcome();
	printlist();
} // ~Runopts::opt_h_Help

void Runopts::opt_v_Verbose(int & narg)
{
	verbose = true;
	narg++;
} // ~Runopts::opt_v_Verbose

void Runopts::opt_N_MatchAmbiguous(char **argv, int &narg)
{
	// match ambiguous N's
	if (!match_ambiguous_N_gv)
	{
		match_ambiguous_N_gv = true;
		score_N = atoi(argv[narg + 1]);
		narg += 2;
	}
	else
	{
		fprintf(stderr, "\n  %sERROR%s: BOOL -N has been set more than once, please "
			"check your command parameters.\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
} // ~Runopts::opt_N_MatchAmbiguous

// TODO: remove
void Runopts::opt_m_MemMapSize(char **argv, int &narg)
{
	// set the map_size_gv variable
	if (!map_size_set_gv)
	{
		// RAM limit for mmap'ing reads in megabytes
		char *pEnd = NULL;
		double _m = strtod(argv[narg + 1], &pEnd);
		// note 1 Mb = 1024^2 = 1048576 bytes
		unsigned long long int pages_asked = \
			(unsigned long long int)(_m * 1048576) / pagesize_gv;
		// RAM limit exceeds available resources
		if (pages_asked > maxpages_gv / 2)
		{
			int max_ram = (maxpages_gv*pagesize_gv) / 1048576;
			fprintf(stderr, "\n  %sERROR%s: [Line %d: %s] -m [INT] must not exceed %d (Mbyte)."
				"\n\n", startColor, "\033[0m", __LINE__, __FILE__, max_ram);
			exit(EXIT_FAILURE);
		}
		// set RAM limit
		if (_m != 0)
		{
			map_size_gv *= pages_asked;
			narg += 2;
			map_size_set_gv = true;
		}
		else
		{
			fprintf(stderr, "\n  %sERROR%s: [Line %d: %s] -m [INT] must be a positive integer "
				"value (in Mbyte).\n\n", startColor, "\033[0m", __LINE__, __FILE__);
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		fprintf(stderr, "\n  %sERROR%s: [Line %d: %s] -m [INT] has been set twice, please verify "
			"your command parameters.\n\n", startColor, "\033[0m", __LINE__, __FILE__);
		exit(EXIT_FAILURE);
	}
} // ~Runopts::opt_m_MemMapSize

void Runopts::opt_Default(char **argv, int &narg)
{
	fprintf(stderr, "\n  %sERROR%s: [Line %d: %s] '%c' is not one of the options.\n",
		startColor, "\033[0m", __LINE__, __FILE__, argv[narg][1]);
	printlist();
} // ~Runopts::opt_Default

void Runopts::optReport(char **argv, int &narg)
{
	std::stringstream ss;
	int reportingOpt = 0;
	sscanf(argv[narg + 1], "%d", &reportingOpt);

	if (reportingOpt > 2) {
		ss << "Optiong report " << reportingOpt << " Can only take values: 0,1,2 Defaulting to 0 ... \n";
		std::cout << ss.str(); ss.str("");
	}

	switch (reportingOpt)
	{
	case 0: alirep = align; break;
	case 1: alirep = report; break;
	case 2: alirep = both; break;
	}

	ss << "ALIGN_REPORT: " << alirep << std::endl;
	std::cout << ss.str();

	narg += 2;
} // ~Runopts::optReport


void Runopts::process(int argc, char**argv, bool dryrun)
{
	if (dryrun) return;

	int narg = 1; // parse the command line input

#if defined(__APPLE__)
	int sz[2] = { CTL_HW, HW_MEMSIZE };
	u_int namelen = sizeof(sz) / sizeof(sz[0]);
	uint64_t size;
	size_t len = sizeof(size);
	if (sysctl(sz, namelen, &size, &len, NULL, 0) < 0)
	{
		fprintf(stderr, "\n  %sERROR%s: sysctl (main.cpp)\n", startColor, "\033[0m");
		exit(EXIT_FAILURE);
	}
	else
	{
		maxpages_gv = size / pagesize_gv;
	}
#else
	maxpages_gv = sysconf(_SC_PHYS_PAGES);
#endif

#if defined(_WIN32)
	_setmode(_fileno(stderr), _O_BINARY);
#endif

	if (argc == 1)
	{
		verbose = true;
		welcome();
		fprintf(stderr, "  For help or more information on usage, type "
			"`./sortmerna %s-h%s'\n\n", "\033[1m", "\033[0m");
		exit(EXIT_FAILURE);
	}

	while (narg < argc)
	{
		switch (argv[narg][1])
		{
		// options beginning with '--'
		case '-':
		{
			char* opt = argv[narg];
			opt += 2; // skip the '--'

			// FASTA/FASTQ reads sequences
			if (strcmp(opt, "reads") == 0) optReads(argv, narg);
#ifdef HAVE_LIBZ
			// FASTA/FASTQ compressed reads sequences
			else if (strcmp(opt, "reads-gz") == 0) optReadsGz(argv, narg);
#endif
			// FASTA reference sequences
			else if (strcmp(opt, "ref") == 0) optRef(argv, narg);
			// the name of output aligned reads
			else if (strcmp(opt, "aligned") == 0) optAligned(argv, narg);
			// the name of output rejected reads
			else if (strcmp(opt, "other") == 0) optOther(argv, narg);
			// output overall statistics file
			else if (strcmp(opt, "log") == 0) optLog(argv, narg);
			// output FASTA/FASTQ reads passing E-value threshold but having < %id 
			// and < %coverage scores for de novo OTU construction
			else if (strcmp(opt, "de_novo_otu") == 0) optDeNovoOtu(argv, narg);
			// output OTU map
			else if (strcmp(opt, "otu_map") == 0) optOtuMap(argv, narg);
			// output non-aligned reads to SAM/BLAST files
			else if (strcmp(opt, "print_all_reads") == 0) optPrintAllReads(argv, narg);
			// don't add pid to output files
			else if (strcmp(opt, "pid") == 0) optPid(argv, narg);
			// put both paired reads into --accept reads file
			else if (strcmp(opt, "paired_in") == 0) optPairedIn(argv, narg);
			// put both paired reads into --other reads file
			else if (strcmp(opt, "paired_out") == 0) optPairedOut(argv, narg);
			// the score for a match
			else if (strcmp(opt, "match") == 0) optMatch(argv, narg);
			// the score for a mismatch
			else if (strcmp(opt, "mismatch") == 0) optMismatch(argv, narg);
			// the score for a gap
			else if (strcmp(opt, "gap_open") == 0) optGapOpen(argv, narg);
			// the score for a gap extension
			else if (strcmp(opt, "gap_ext") == 0) optGapExt(argv, narg);
			// number of seed hits before searching for candidate LCS
			else if (strcmp(opt, "num_seeds") == 0) optNumSeeds(argv, narg);
			// output all hits in FASTX format
			else if (strcmp(opt, "fastx") == 0) optFastx(argv, narg);
			// output all hits in SAM format
			else if (strcmp(opt, "sam") == 0) optSam(argv, narg);
			// output all hits in BLAST format
			else if (strcmp(opt, "blast") == 0) optBlast(argv, narg);
			// output best alignment as predicted by the longest increasing subsequence
			else if (strcmp(opt, "min_lis") == 0) optMinLis(argv, narg);
			// output best alignment as predicted by the longest increasing subsequence
			else if (strcmp(opt, "best") == 0) optBest(argv, narg);
			// output all alignments
			else if (strcmp(opt, "num_alignments") == 0) optNumAlignments(argv, narg);
			// number of nucleotides to add to each edge of an alignment region before extension
			else if (strcmp(opt, "edges") == 0) optEdges(argv, narg);
			// execute full index search for 0-error and 1-error seed matches
			else if (strcmp(opt, "full_search") == 0) optFullSearch(argv, narg);
			// do not output SQ tags in the SAM file
			else if (strcmp(opt, "SQ") == 0) optSQ(argv, narg);
			// --passes option
			else if (strcmp(opt, "passes") == 0) optPasses(argv, narg);
			else if (strcmp(opt, "id") == 0) optId(argv, narg);
			else if (strcmp(opt, "coverage") == 0) optCoverage(argv, narg);
			// the version number
			else if (strcmp(opt, "version") == 0) optVersion(argv, narg);
			else if (strcmp(opt, "report") == 0) optReport(argv, narg);
			else optUnknown(argv, narg, opt);
		}
		break;
		case 'a': opt_a_NumCpus(argv, narg); break;
		case 'e': opt_e_Evalue(argv, narg);	break;
		case 'F': opt_F_ForwardOnly(argv, narg); break;
		case 'R': opt_R_ReverseOnly(argv, narg); break;
		case 'h': opt_h_Help(); break;
		case 'v': opt_v_Verbose(narg); break;
		case 'N': opt_N_MatchAmbiguous(argv, narg); break;
		case 'm': opt_m_MemMapSize(argv, narg); break; // TODO: remove
		default: opt_Default(argv, narg);
		}//~switch
	}//~while ( narg < argc )



	 // ERROR messages ******* 
	 // Reads file is mandatory
	if (readsfile.empty() || indexfiles.empty())
	{
		fprintf(stderr, "\n  %sERROR%s: [Line %d: %s] a reads file (--reads file.{fa/fq}) and a "
			"reference sequence file (--ref /path/to/file1.fasta,/path/to/index1) "
			"are mandatory input.\n\n", startColor, "\033[0m", __LINE__, __FILE__);
		printlist();
	}
	// Basename for aligned reads is mandatory
	if (ptr_filetype_ar == NULL)
	{
		fprintf(stderr, "\n  %sERROR%s: [Line %d: %s] parameter --aligned [STRING] is mandatory.\n\n",
			startColor, "\033[0m", __LINE__, __FILE__);
		exit(EXIT_FAILURE);
	}
	// No output format has been chosen
	else if (!(fastxout_gv || blastout || samout || otumapout_gv || logout_gv || de_novo_otu_gv))
	{
		fprintf(stderr, "\n  %sERROR%s: [Line %d: %s] no output format has been chosen (fastx/sam/blast/otu_"
			"map/log).\n\n", startColor, "\033[0m", __LINE__, __FILE__);
		exit(EXIT_FAILURE);
	}
	// Options --paired_in and --paired_out can only be used with FASTA/Q output
	if (!fastxout_gv && (pairedin_gv || pairedout_gv))
	{
		fprintf(stderr, "\n  %sERROR%s: [Line %d: %s] options --paired_in and --paired_out "
			"must be accompanied by option --fastx.\n", startColor, "\033[0m", __LINE__, __FILE__);
		fprintf(stderr, "  These BOOLs are for FASTA and FASTQ output files, for "
			"maintaining paired reads together.\n");
		exit(EXIT_FAILURE);
	}
	// Basename for non-aligned reads is mandatory
	if (ptr_filetype_or != NULL)
	{
		if (!fastxout_gv && (blastout || samout))
		{
			fprintf(stderr, "\n  %sERROR%s: [Line %d: %s] option --other [STRING] can only be used together "
				"with the --fastx option.\n\n", startColor, "\033[0m", __LINE__, __FILE__);
			exit(EXIT_FAILURE);
		}
	}
	// An OTU map can only be constructed with the single best alignment per read
	if (otumapout_gv && num_alignments_gv_set)
	{
		fprintf(stderr, "\n  %sERROR%s: [Line %d: %s] --otu_map cannot be set together with "
			"--num_alignments [INT].\n", startColor, "\033[0m", __LINE__, __FILE__);
		fprintf(stderr, "  The option --num_alignments [INT] doesn't keep track of "
			"the best alignment which is required for constructing an OTU map.\n");
		fprintf(stderr, "  Use --otu_map with --best [INT] instead.\n\n");
		exit(EXIT_FAILURE);
	}
	// If --num_alignments output was chosen, check an alignment format has also been chosen
	if (num_alignments_gv_set && !(blastout || samout || fastxout_gv))
	{
		fprintf(stderr, "\n  %sERROR%s: [Line %d: %s] --num_alignments [INT] has been set but no output "
			"format has been chosen (--blast, --sam or --fastx).\n\n", startColor, "\033[0m", __LINE__, __FILE__);
		exit(EXIT_FAILURE);
	}
	// If --best output was chosen, check an alignment format has also been chosen
	if (best_gv_set && !(blastout || samout || otumapout_gv))
	{
		fprintf(stderr, "\n  %sERROR%s: [Line %d: %s] --best [INT] has been set but no output "
			"format has been chosen (--blast or --sam or --otu_map).\n\n", startColor, "\033[0m", __LINE__, __FILE__);
		exit(EXIT_FAILURE);
	}
	// Check gap extend score < gap open score
	if (gap_extension > gap_open)
	{
		fprintf(stderr, "\n  %sERROR%s: [Line %d: %s] --gap_ext [INT] must be less than --gap_open [INT].\n\n",
			startColor, "\033[0m", __LINE__, __FILE__);
		exit(EXIT_FAILURE);
	}
	// Option --print_all_reads can only be used with Blast-like tabular
	// and SAM formats (not pairwise)
	if (print_all_reads_gv && blastout && !blast_tabular)
	{
		fprintf(stderr, "\n  %sERROR%s: [Line %d: %s] --print_all_reads [BOOL] can only be used with BLAST-like "
			"tabular format.\n\n", startColor, "\033[0m", __LINE__, __FILE__);
		exit(EXIT_FAILURE);
	}
	// Only one of these options is allowed (--best outputs one alignment,
	// --num_alignments outputs > 1 alignments)
	if (best_gv_set && num_alignments_gv_set)
	{
		fprintf(stderr, "\n  %sERROR%s: [Line %d: %s] --best [INT] and --num_alignments [INT] cannot "
			"be set together. \n", startColor, "\033[0m", __LINE__, __FILE__);
		fprintf(stderr, "  (--best [INT] will search INT highest scoring reference sequences "
			"and output a single best alignment, whereas --num_alignments [INT] will "
			"output the first INT alignments).\n\n");
	}
	// Option --min_lis [INT] can only accompany --best [INT]
	if (min_lis_gv_set && num_alignments_gv_set)
	{
		fprintf(stderr, "\n  %sERROR%s: [Line %d: %s] --min_lis [INT] and --num_alignments [INT] cannot "
			"be set together. \n", startColor, "\033[0m", __LINE__, __FILE__);
		fprintf(stderr, "  --min_lis [INT] can only be used with --best [INT] (refer to "
			"the User manual on this option).\n\n");
		exit(EXIT_FAILURE);
	}
	// Option --mis_lis INT accompanies --best INT, cannot be set alone
	if (min_lis_gv_set && !best_gv_set)
	{
		fprintf(stderr, "\n  %sERROR%s: [Line %d: %s] --min_lis [INT] must be set together with --best "
			"[INT].\n\n", startColor, "\033[0m", __LINE__, __FILE__);
		exit(EXIT_FAILURE);
	}
	// %id and %coverage can only be used with --otu_map
	if (((align_id > 0) || (align_cov > 0)) && !otumapout_gv)
	{
		fprintf(stderr, "\n  %sERROR%s: [Line %d: %s] --id [INT] and --coverage [INT] can only be used "
			"together with --otu_map.\n", startColor, "\033[0m", __LINE__, __FILE__);
		fprintf(stderr, "  These two options are used for constructing the OTU map by "
			"filtering alignments passing the E-value threshold.\n\n");
		exit(EXIT_FAILURE);
	}
	// reads input is in compressed format and mmap was chosen
	if (map_size_set_gv && have_reads_gz)
	{
		fprintf(stderr, "\n  %sWARNING%s: [Line %d: %s] loading compressed reads using memory map "
			"is not possible. Using default buffer I/O.\n", "\033[0;33m", "\033[0m", __LINE__, __FILE__);
		map_size_set_gv = false;
	}
	// the list of arguments is correct, welcome the user!
	if (verbose) welcome();
	// if neither strand was selected for search, search both
	if (!forward && !reverse)
	{
		forward = true;
		reverse = true;
	}
	// default number of threads is 1
	if (numcpu_gv < 0) numcpu_gv = 1;
	// default E-value
	if (evalue < 0.0) evalue = 1;
	// SW alignment parameters
	if (!match_set) match = 2;
	if (!mismatch_set) mismatch = -3;
	if (!gap_open_set) gap_open = 5;
	if (!gap_ext_set) gap_extension = 2;
	if (!match_ambiguous_N_gv) score_N = mismatch;
	// default method for searching alignments
	if (!best_gv_set && !num_alignments_gv_set)
	{
		// FASTA/FASTQ output, stop searching for
		// alignments after the first match
		if (fastxout_gv && !(blastout || samout || otumapout_gv || logout_gv || de_novo_otu_gv))
			num_alignments_gv = 1;
		// output single best alignment from best candidate hits
		else
		{
			num_best_hits_gv = 1;
			min_lis_gv = 2;
		}
	}
	// default minimum LIS used for setting the number of
	// alignments to search prior to outputting --best INT
	if (best_gv_set && !min_lis_gv_set) min_lis_gv = 2;
	// default number of seed hits before searching for candidate LIS
	if (seed_hits_gv < 0) seed_hits_gv = 2;
	// default number of nucleotides to add to each edge of an alignment
	// region before extension
	if (edges_gv < 0) edges_gv = 4;
	// activate heuristic for stopping search (of 1-error matches) after
	// finding 0-error match
	if (!full_search_set) full_search_gv = false;
	// default %id to keep alignment
	if (align_id < 0)
	{
		// if OTU-map is chosen, set default similarity to 0.97
		if (otumapout_gv) align_id = 0.97;
		else align_id = 0;
	}
	// default %query coverage to keep alignment
	if (align_cov < 0)
	{
		// if OTU-map is chosen, set default coverage to 0.97
		if (otumapout_gv) align_cov = 0.97;
		else align_cov = 0;
	}

	for (int i = 0; i < argc; i++) {
		cmdline.append(argv[i]);
		cmdline.append(" ");
	}
} // ~Runopts::process


/*! @fn main()
	@brief main function, parses command line arguments and launches the
	main paralleltraversal function
	@param int argc
	@param char** argv
	@return none
 */
int main(int argc, char** argv)
{
	bool dryrun = false;
	Runopts opts(argc, argv, dryrun);
	switch (opts.alirep)
	{
	case Runopts::ALIGN_REPORT::align: paralleltraversal(opts); break;
	case Runopts::ALIGN_REPORT::report: generateReports(opts); break;
	case Runopts::ALIGN_REPORT::both: paralleltraversal(opts); generateReports(opts); break;
	}

	return 0;
}//~main()
