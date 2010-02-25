#ifndef _PSI_SRC_LIB_LIBDIIS_DIISENTRY_H_
#define _PSI_SRC_LIB_LIBDIIS_DIISENTRY_H_

#include <libdpd/dpd.h>
#include <libpsio/psio.hpp>
#include "psifiles.h"

using namespace psi;

namespace psi{ namespace libdiis{

  /**
   * @Brief The DIISManager class is used to manage DIIS quantities and
   * their corresponding error vectors.
   */
  
class DIISEntry{
    public:
        /**
         * @brief The type of quantity to add to either an error vector or vector.
         *
         * DPDBuf4  - A pointer to a four-index DPD quantity, assumed to be initialized
         *            via dpd_buf4_init() already.
         * DPDFile2 - A pointer to a two-index DPD quality, assumed to be initialized
         *            via dpd_file2_init() already.
         * Matrix   - A pointer to a Psi Matrix object.
         * Vector   - A pointer to a Psi Vector object.
         * Pointer  - The quantity will be taken from contiguous memory pointed to by a
         *            double* pointer.  In this case, the next parameter passed to the
         *            set_size.. routines should be the number of elements to read.
         */
        enum InputType {DPDBuf4, DPDFile2, Matrix, Vector, Pointer};
        DIISEntry(int ID, int count, int vectorSize, double *vector,
                  int errorVectorSize, double *errorVector);
        ~DIISEntry();
        /// Whether the dot product of this entry's and the nth entry's error vector is known
        bool dot_is_known_with(int n) {return _knownDotProducts[n];}
        /// The dot product of this entry's and the nth entry's error vectors
        double dot_with(int n) {return _dotProducts[n];}
        /// The RMS error of this entry
        double rmsError() {return _rmsError;}
        /// The absolute number of this entry
        int orderAdded() {return _orderAdded;}
        /// Sets the dot product with vector n to val
        void set_dot_with(int n, double val) {_knownDotProducts[n] = true; _dotProducts[n] = val;}
        /// Marks the dot product with vector n as invalid
        void invalidate_dot(int n) {_knownDotProducts[n] = false;}
        /// Set the vector
        void set_vector(double *vec) {_vector = vec;}
        /// set the error vector
        void set_error_vector(double *vec) {_errorVector = vec;}
        /// Put this entry on disk and free the memory
        void dump_to_disk();
        /// Allocate memory and read from disk
        void read_from_disk();
        /// Returns the error vector
        const double *errorVector() {read_from_disk(); return _errorVector;}
        /// Returns the vector
        const double *vector() {read_from_disk(); return _vector;}
    protected:
        /// The list of which dot products, with other DIISEntries, are known
        std::map<int, bool> _knownDotProducts;
        /// The list of known dot products with other DIISEntries
        std::map<int, double> _dotProducts;
        /// The length of the error vector
        int _errorVectorSize;
        /// The length of the vector
        int _vectorSize;
        /// The absolute number of this entry
        int _orderAdded;
        /// The number of this entry in the current subspace
        int _ID;
        /// The RMS error for this entry
        double _rmsError;
        /// The error vector
        double *_errorVector;
        /// The error vector
        double *_vector;
};

}} // End namespaces

// This is here so that files including this have clean(er) syntax
using namespace psi::libdiis;

#endif // Header guard