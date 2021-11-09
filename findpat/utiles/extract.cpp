#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <map>
#include <fstream>
#include <iostream>
#include <set>

using namespace std;

typedef unsigned int uint;
#define forsn(i, s, n)	for (uint i = (s); i < (n); i++)
#define forn(i, n)	forsn (i, 0, n)

#define TAMBUF		65536

char buf[TAMBUF];
const char separator = 0x7e;

int read_write_fasta(FILE *f, FILE *g, int *is_good) {
	bool start_line = true;
	bool in_header = false;
	bool separator_pending = false;
	uint inpos = 0;

	uint rd = TAMBUF;
	while (rd == TAMBUF) {
		rd = fread(buf, sizeof(char), TAMBUF, f);
		forn (i, rd) {
			if (in_header) {
				if (buf[i] == '\n') {
					start_line = true;
					in_header = false;
				} /* else skip */
			} else {
				assert(buf[i] != ' ' && buf[i] != '\t' && buf[i] != '\r');
				if (start_line && buf[i] == '>') {
					in_header = true;
					start_line = false;
				} else if (buf[i] == '\n') {
					start_line = true;
				} else {
					if (is_good[inpos]) {
						fputc(buf[i], g);
						separator_pending = true;
					} else if (separator_pending) {
						fputc(separator, g);
						separator_pending = false;
					}
					inpos++;
					start_line = false;
				}
			}
		}
	}
}

map<string, uint> chromosome_size;
map<string, string> fasta_prefix;
const string in_dir = "in-fasta/";
const string out_dir = "fasta/";
const string db_dir = "db-javito/";
void init() {
	/* read sizes */
	ifstream is("sizes_all.txt");
	while (!is.eof()) {
		string chrom;
		is >> chrom;
		if (chrom != "") is >> chromosome_size[chrom];
	}
	is.close();

	/* fasta prefixes */
	fasta_prefix["homo"] = "Homo_sapiens-GRCh37-chromosome";
	fasta_prefix["bos"] = "Bos_taurus.Btau_4.0.53.dna.chromosome";
	fasta_prefix["canis"] = "Canis_familiaris.BROADD2.53.dna.chromosome";
	fasta_prefix["equus"] = "Equus_caballus.EquCab2.53.dna.chromosome";
	fasta_prefix["gallus"] = "Gallus_gallus.WASHUC2.53.dna.chromosome";
	fasta_prefix["macaca"] = "Macaca_mulatta.MMUL_1.53.dna.chromosome";
	fasta_prefix["mus"] = "Mus_musculus.NCBIM37.53.dna.chromosome";
	fasta_prefix["pan"] = "Pan_troglodytes.CHIMP2.1.53.dna.chromosome";
	fasta_prefix["rattus"] = "Rattus_norvegicus.RGSC3.4.53.dna.chromosome";
}

void filter(const string& species, const string& chromosome, const set<string>* classes, const string& filetype, const string& cls_id) {
	const string filter_file = db_dir + filetype + "." + species + ".txt";

	const string fasta_basename = fasta_prefix[species] + "." + chromosome + ".fa";
	const string fasta_file = in_dir + fasta_basename;

	const string out_basename = species + "." + chromosome + "." + cls_id + ".fa";
	const string out_file = out_dir + out_basename;

	uint n = chromosome_size[fasta_basename];

	int *delta = (int *)malloc(sizeof(int) * (n + 1));
	bzero(delta, sizeof(int) * (n + 1));

	cerr << "reading db" << endl;
	ifstream is(filter_file.c_str());
	string tmp; getline(is, tmp); /* discard */

	string _chr, _start, _end, _strand, _class, _name;
	is >> _chr >> _start >> _end >> _strand >> _class >> _name;
	assert(_chr == "CHR");
	assert(_start == "START");
	assert(_end == "END");
	assert(_strand == "STRAND");
	assert(_class == "CLASS");
	assert(_name == "NAME");
	while (!is.eof()) {
		is >> _chr >> _start >> _end >> _strand >> _class >> _name;
		if (_chr != chromosome) continue;
		if (classes != NULL && classes->find(_class) == classes->end()) continue;

		uint s = atoi(_start.c_str());
		uint e = atoi(_end.c_str());
		if (s >= n || e >= n || s > e) {
			fprintf(stderr, "warning: interval %u--%u out of bounds\n", s, e);
			continue;
		}
		delta[s]++;
		delta[e + 1]--;
	}

	/* acumulo */
	cerr << "accumulating" << endl;
	forn (i, n) delta[i + 1] += delta[i];

	cerr << "reading input / writing output" << endl;
	cout << "\t" << fasta_file << endl;
	cout << "\t" << out_file << endl;
	FILE *f = fopen(fasta_file.c_str(), "r");
	if (!f) {
		fprintf(stderr, "error: cannot open %s for reading\n", fasta_file.c_str());
		exit(1);
	}
	FILE *g = fopen(out_file.c_str(), "w");
	if (!g) {
		fprintf(stderr, "error: cannot open %s for writing\n", out_file.c_str());
		exit(1);
	}
	read_write_fasta(f, g, delta);
	fclose(f);
	fclose(g);

	free(delta);
	is.close();
}

int main(int argc, char **argv) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s species chromosome\n", argv[0]);
		return 1;
	}

	init();

	string species(argv[1]);
	string chromosome(argv[2]);

#if 0

	{
	// LTR
	set<string> ltr;
	ltr.insert("LTR/ERVL");
	ltr.insert("LTR/ERV1");
	ltr.insert("LTR/MaLR");
	ltr.insert("LTR/ERV");
	ltr.insert("LTR/ERVK");
	ltr.insert("LTR/ERV1?");
	ltr.insert("LTR/ERVL?");
	ltr.insert("LTR");
	ltr.insert("LTR/Gypsy");
	ltr.insert("LTR/Gypsy?");
	ltr.insert("LTR?");
	filter(species, chromosome, &ltr, "repeats", "ltr");
	}

	{
	// Satellite
	set<string> satellite;
	satellite.insert("Satellite");
	satellite.insert("Satellite/centr");
	satellite.insert("Satellite/telo");
	satellite.insert("Satellite/macro");
	satellite.insert("Satellite/Y-chromosome");
	satellite.insert("Satellite?/telomeric?");
	satellite.insert("Satellite/W-chromosome");
	satellite.insert("Satellite/acro");
	filter(species, chromosome, &satellite, "repeats", "satellite");
	}

	{
	// LINE/L1
	set<string> line_l1;
	line_l1.insert("LINE/L1");
	filter(species, chromosome, &line_l1, "repeats", "line_l1");
	}

	{
	// SINE/Alu
	set<string> sine_alu;
	sine_alu.insert("SINE/Alu");
	filter(species, chromosome, &sine_alu, "repeats", "sine_alu");
	}

	{
	// genes
	filter(species, chromosome, NULL, "genes", "genes");
	}
	{
	// exons
	filter(species, chromosome, NULL, "exons", "exons");
	}
#endif

	{
		// DNA
		set<string> dna;
		dna.insert("DNA");
		filter(species, chromosome, &dna, "repeats", "dna");
	}

	{
		// DNA/AcHobo
		set<string> dna_achobo;
		dna_achobo.insert("DNA/AcHobo");
		filter(species, chromosome, &dna_achobo, "repeats", "dna_achobo");
	}

	{
		// DNA/Charlie
		set<string> dna_charlie;
		dna_charlie.insert("DNA/Charlie");
		filter(species, chromosome, &dna_charlie, "repeats", "dna_charlie");
	}

	{
		// DNA/MER1_type
		set<string> dna_mer1_type;
		dna_mer1_type.insert("DNA/MER1_type");
		filter(species, chromosome, &dna_mer1_type, "repeats", "dna_mer1_type");
	}

	{
		// DNA/MER2_type
		set<string> dna_mer2_type;
		dna_mer2_type.insert("DNA/MER2_type");
		filter(species, chromosome, &dna_mer2_type, "repeats", "dna_mer2_type");
	}

	{
		// DNA/Mariner
		set<string> dna_mariner;
		dna_mariner.insert("DNA/Mariner");
		filter(species, chromosome, &dna_mariner, "repeats", "dna_mariner");
	}

	{
		// DNA/Merlin
		set<string> dna_merlin;
		dna_merlin.insert("DNA/Merlin");
		filter(species, chromosome, &dna_merlin, "repeats", "dna_merlin");
	}

	{
		// DNA/MuDR
		set<string> dna_mudr;
		dna_mudr.insert("DNA/MuDR");
		filter(species, chromosome, &dna_mudr, "repeats", "dna_mudr");
	}

	{
		// DNA/PiggyBac, DNA/piggyBac
		set<string> dna_piggybac;
		dna_piggybac.insert("DNA/PiggyBac");
		dna_piggybac.insert("DNA/piggyBac");
		filter(species, chromosome, &dna_piggybac, "repeats", "dna_piggybac");
	}

	{
		// DNA/Tc2
		set<string> dna_tc2;
		dna_tc2.insert("DNA/Tc2");
		filter(species, chromosome, &dna_tc2, "repeats", "dna_tc2");
	}

	{
		// DNA/TcMar
		set<string> dna_tcmar;
		dna_tcmar.insert("DNA/TcMar");
		filter(species, chromosome, &dna_tcmar, "repeats", "dna_tcmar");
	}

	{
		// DNA/Tigger
		set<string> dna_tigger;
		dna_tigger.insert("DNA/Tigger");
		filter(species, chromosome, &dna_tigger, "repeats", "dna_tigger");
	}

	{
		// DNA/Tip100
		set<string> dna_tip100;
		dna_tip100.insert("DNA/Tip100");
		filter(species, chromosome, &dna_tip100, "repeats", "dna_tip100");
	}

	{
		// DNA/hAT, DNA/hAT-Blackjack, DNA/hAT-Tip100
		set<string> dna_hat;
		dna_hat.insert("DNA/hAT");
		dna_hat.insert("DNA/hAT-Blackjack");
		dna_hat.insert("DNA/hAT-Tip100");
		filter(species, chromosome, &dna_hat, "repeats", "dna_hat");
	}

	{
		// LINE/CR1
		set<string> line_cr1;
		line_cr1.insert("LINE/CR1");
		filter(species, chromosome, &line_cr1, "repeats", "line_cr1");
	}

	{
		// LINE/Dong-R4
		set<string> line_dong_r4;
		line_dong_r4.insert("LINE/Dong-R4");
		filter(species, chromosome, &line_dong_r4, "repeats", "line_dong_r4");
	}

	{
		// LINE/L2
		set<string> line_l2;
		line_l2.insert("LINE/L2");
		filter(species, chromosome, &line_l2, "repeats", "line_l2");
	}

	{
		// LINE/RTE, LINE/RTE-BovB
		set<string> line_rte;
		line_rte.insert("LINE/RTE");
		line_rte.insert("LINE/RTE-BovB");
		filter(species, chromosome, &line_rte, "repeats", "line_rte");
	}

	{
		// RC/Helitron
		set<string> rc_helitron;
		rc_helitron.insert("RC/Helitron");
		filter(species, chromosome, &rc_helitron, "repeats", "rc_helitron");
	}

	{
		// RNA
		set<string> rna;
		rna.insert("RNA");
		filter(species, chromosome, &rna, "repeats", "rna");
	}

	{
		// SINE
		set<string> sine;
		sine.insert("SINE");
		filter(species, chromosome, &sine, "repeats", "sine");
	}

	{
		// SINE/B2
		set<string> sine_b2;
		sine_b2.insert("SINE/B2");
		filter(species, chromosome, &sine_b2, "repeats", "sine_b2");
	}

	{
		// SINE/B4
		set<string> sine_b4;
		sine_b4.insert("SINE/B4");
		filter(species, chromosome, &sine_b4, "repeats", "sine_b4");
	}

	{
		// SINE/BovA
		set<string> sine_bova;
		sine_bova.insert("SINE/BovA");
		filter(species, chromosome, &sine_bova, "repeats", "sine_bova");
	}

	{
		// SINE/Deu
		set<string> sine_deu;
		sine_deu.insert("SINE/Deu");
		filter(species, chromosome, &sine_deu, "repeats", "sine_deu");
	}

	{
		// SINE/ID
		set<string> sine_id;
		sine_id.insert("SINE/ID");
		filter(species, chromosome, &sine_id, "repeats", "sine_id");
	}

	{
		// SINE/Lys
		set<string> sine_lys;
		sine_lys.insert("SINE/Lys");
		filter(species, chromosome, &sine_lys, "repeats", "sine_lys");
	}

	{
		// SINE/MIR
		set<string> sine_mir;
		sine_mir.insert("SINE/MIR");
		filter(species, chromosome, &sine_mir, "repeats", "sine_mir");
	}

	{
		// SINE/tRNA, SINE/tRNA-Glu
		set<string> sine_trna;
		sine_trna.insert("SINE/tRNA");
		sine_trna.insert("SINE/tRNA-Glu");
		filter(species, chromosome, &sine_trna, "repeats", "sine_trna");
	}

	{
		// rRNA
		set<string> rrna;
		rrna.insert("rRNA");
		filter(species, chromosome, &rrna, "repeats", "rrna");
	}

	{
		// scRNA
		set<string> scrna;
		scrna.insert("scRNA");
		filter(species, chromosome, &scrna, "repeats", "scrna");
	}

	{
		// snRNA
		set<string> snrna;
		snrna.insert("snRNA");
		filter(species, chromosome, &snrna, "repeats", "snrna");
	}

	{
		// srpRNA
		set<string> srprna;
		srprna.insert("srpRNA");
		filter(species, chromosome, &srprna, "repeats", "srprna");
	}

	{
		// tRNA
		set<string> trna;
		trna.insert("tRNA");
		filter(species, chromosome, &trna, "repeats", "trna");
	}

	{
		// trf
		set<string> trf;
		trf.insert("trf");
		filter(species, chromosome, &trf, "repeats", "trf");
	}

	return 0;
}

