#include <cstdio>
#include "basisset_parser.h"

#include <fstream>
#include <algorithm>

#include <boost/regex.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <boost/xpressive/regex_actions.hpp>
#include <boost/algorithm/string.hpp>

using namespace psi;
using namespace boost;

// the third parameter of from_string() should be
// one of std::hex, std::dec or std::oct
template <class T>
bool from_string(T& t,
                 const std::string& s,
                 std::ios_base& (*f)(std::ios_base&))
{
    std::istringstream iss(s);
    return !(iss >> f >> t).fail();
}

BasisSetParser::BasisSetParser(const std::string& searchpath)
   : searchpath_(searchpath)
{
    // If the search path is empty use either PSIDATADIR or INSTALLPSIDATADIR
    if (searchpath_.empty()) {
        std::string psiDataDirName;
        if (getenv("PSIDATADIR"))
            psiDataDirName = getenv("PSIDATADIR");
        if (psiDataDirName.empty())
            psiDataDirName = INSTALLEDPSIDATADIR;
        psiDataDirName += "/basis";
        searchpath_ = psiDataDirName;
    }
}

BasisSetParser::~BasisSetParser()
{
}

void Gaussian94BasisSetParser::parse(shared_ptr<BasisSet>& basisSet, const vector<string> &basisnames)
{
    // Obtain hold of molecule object from the basis set, we'l need to walk throught this
    shared_ptr<Molecule> molecule = basisSet->molecule();

    // Ensure that the number of atoms match the number of basis names
    if (molecule->natom() != basisnames.size())
        throw PSIEXCEPTION("Gaussian94BasisSetParser::parse: Number of atoms does not match number of basis set names.");

    // Regular expressions that we'll be checking for.
    regex comment("^\\s*\\!.*");                                    // line starts with !
    regex separator("^\\*\\*\\*\\*");                               // line starts with ****
    regex atom_array("^\\s*(?:([A-Za-z]+)\\s+)+0.*");               // array of atomic symbols terminated by 0
    regex shell("^\\s*(\\w+)\\s*(\\d+)\\s*(-?\\d+\\.\\d+)");        // Match beginning of contraction
    regex primitives1("^\\s*(-?\\d+\\.\\d+)\\s*(-?\\d+\\.\\d+).*");    // Match s, p, d, f, g, ... functions
    regex primitives2("^\\s*(-?\\d+\\.\\d+)\\s*(-?\\d+\\.\\d+)\\s*(-?\\d+\\.\\d+).*"); // match sp functions
    regex primitives3("^\\s*(-?\\d+\\.\\d+)\\s*(-?\\d+\\.\\d+)\\s*(-?\\d+\\.\\d+)\\s*(-?\\d+\\.\\d+).*"); // match spd functions

    // s, p and s, p, d can be grouped together in Pople-style basis sets
    const string sp("SP"), spd("SPD");

    //                     a  b  c  d  e  f  g  h  i  j  k  l  m  n  o  p  q  r  s  t  u  v  w  x  y  z
    char shell_to_am[] = {-1,-1,-1, 2,-1, 3, 4, 5, 6,-1, 7, 8, 9,10,11, 1,12,13, 0,14,15,16,17,18,19,20};

    // Hold the result of a regex_match
    smatch what;

    int atom;
    for (atom=0; atom<molecule->natom(); ++atom) {
        // Modify the name of the basis set to generate a filename: STO-3G -> sto3g
        string basisname = basisnames[atom];
        // First make it lower case
        transform(basisname.begin(), basisname.end(), basisname.begin(), ::tolower);
        // Remove all '-'
        xpressive::sregex match_hyphen = xpressive::as_xpr("-");
        string format_hyphen; // empty string
        basisname = regex_replace(basisname, match_hyphen, format_hyphen);

        cout << " basisname with '-' removed: " << basisname << endl;
        string basis_filename = searchpath() + "/" + basisname + ".gbs";
        cout << " will attempt to read from: " << basis_filename << endl;

        ifstream infile(basis_filename.c_str());
        string line;

        if (!infile)
            throw PSIEXCEPTION("Gaussian94BasisSetParser::parse: Unable to open basis set file: " + basis_filename);

        while (infile.good()) {
            // Get a line from the file.
            getline(infile, line);

            // Spit out the line for debugging.
            cout << line << endl;

            // Ignore blank lines
            if (line.empty())
                continue;

            // Do some matches
            if (regex_match(line, what, comment)) {
                cout << " matched a comment line.\n";
                continue;
            }
            if (regex_match(line, what, separator)) {
                cout << " matched a separator line.\n";
                continue;
            }
            // Match: H    0
            // or:    H    O...     0
            if (regex_match(line, what, atom_array, boost::match_extra)) {
                cout << " matched a atom array line.\n";
                for(int i = 0; i < what.size(); ++i)
                    std::cout << "      $" << i << " = \"" << what[i] << "\"\n";
                std::cout << "   Captures:\n";
                for(int i = 0; i < what.size(); ++i)
                {
                    std::cout << "      $" << i << " = {";
                    for(int j = 0; j < what.captures(i).size(); ++j)
                    {
                        if(j)
                            std::cout << ", ";
                        else
                            std::cout << " ";
                        std::cout << "\"" << what.captures(i)[j] << "\"";
                    }
                    std::cout << " }\n";
                }
                // Check the captures and see if this basis set is for the atom we need.
                bool found = false;
                for (int i=0; i < what.captures(1).size(); ++i) {
                    if (iequals(molecule->label(atom), string(what.captures(1)[i].first, what.captures(1)[i].second)))
                        found = true;
                }

                if (found) {
                    cout << "found\n";

                    // Read in the next line
                    getline(infile, line);

                    // Match shell information
                    if (regex_match(line, what, shell)) {
                        string shell_type(what[1].first, what[1].second);
                        int nprimitive;
                        double scale;
                        double exponent, contraction;

                        if (!from_string<int>(nprimitive, what[2], std::dec))
                            throw PSIEXCEPTION("Gaussian94BasisSetParser::parse: Unable to convert number of primitives:\n" + line);
                        if (!from_string<double>(scale, what[3], std::dec))
                            throw PSIEXCEPTION("Gaussian94BasisSetParser::parse: Unable to convert scale factor:\n" + line);

                        if (shell_type.size() == 1) {
                            int *am = new int[1];
                            am[0] = (int)shell_to_am[shell_type[0] - 'A'];

                            double *exponents = new double[nprimitive];
                            double **contractions = new double*[nprimitive];
                            for (int i=0; i<nprimitive; ++i)
                                contractions[i] = new double[1];

                            for (int p=0; p<nprimitive; ++p) {
                                getline(infile, line);

                                // Must match primitives1;
                                if (!regex_match(line, what, primitives1))
                                    throw PSIEXCEPTION("Gaussian94BasisSetParser::parse: Unable to match an exponent with one contraction:\n" + line);

                                if (!from_string<double>(exponent, what[1], std::dec))
                                    throw PSIEXCEPTION("Gaussian94BasisSetParser::parse: Unable to convert exponent:\n" + line);
                                if (!from_string<double>(contraction, what[1], std::dec))
                                    throw PSIEXCEPTION("Gaussian94BasisSetParser::parse: Unable to convert contraction:\n" + line);

                                // Scale the contraction
                                contraction *= scale;

                                // Save the information.
                                exponents[p] = exponent;
                                contractions[p][0] = contraction;
                            }

                            // We have a full shell, push it to the basis set
//                            shared_ptr<GaussianShell> new_shell(new GaussianShel);
//                            new_shell->init(1, nprimitive, exponents, am, GaussianShell::Pure, contractions, atom, molecule->xyz(atom), puream);
//                            basisSet->shells_.push_back(new_shell);
                        }
                    } else {
                        throw PSIEXCEPTION("Gaussian94BasisSetParser::parse: Expected shell information, but got:\n" + line);
                    }
                }
            }
            if (regex_match(line, what, shell)) {
                cout << " matched a function line.\n";
            }
        }
    }
}
