#ifndef _psi_src_lib_libmints_properties_h_
#define _psi_src_lib_libmints_properties_h_
#include <libmints/basisset.h>
#include <libmints/basispoints.h>
#include <libmints/matrix.h>
#include <libmints/vector3.h>
#include <libutil/ref.h>
namespace psi {

	class Properties:public BasisPoints
	{
		protected:
			double* mos_;
			int* mo_inds_;
			int nmo_;
			double density_;
			double densityX_;
			double densityY_;
			double densityZ_;
			double densityXY_;
			double densityXZ_;
			double densityYZ_;
			double densityXX_;
			double densityYY_;
			double densityZZ_;
			double density_gradient_;
			double density_laplacian_;
			double ke_density_;
			bool do_mos_;
			bool do_density_;
			bool do_density_gradient_;
			bool do_density_hessian_;
			bool do_density_laplacian_;
			bool do_ke_density_;
		public:
			static Properties * constructProperties(shared_ptr<BasisSet> b)
			{
				return new Properties(b);
			}
			Properties(shared_ptr<BasisSet> b);
			~Properties();
			void computeProperties(Vector3 v, SharedMatrix D, SharedMatrix C = SharedMatrix() );
			const double getDensity() const { return density_; }		
			const double getDensityX() const { return densityX_; }		
			const double getDensityY() const { return densityY_; }		
			const double getDensityZ() const { return densityZ_; }		
			const double getDensityGradientMagnitude() const { return density_gradient_; }
			const double getDensityXY() const { return densityXY_; }		
			const double getDensityXZ() const { return densityXZ_; }		
			const double getDensityYZ() const { return densityYZ_; }		
			const double getDensityXX() const { return densityXX_; }		
			const double getDensityYY() const { return densityYY_; }		
			const double getDensityZZ() const { return densityZZ_; }		
			const double getDensityLaplacian() const { return density_laplacian_; }
			const double getKEDensity() const { return ke_density_; }	
			const double getMO(int i) const {return mos_[i]; }
			void setToComputeMOs(bool v, int* indices, int n);
			void setToComputeDensity(bool v);
			void setToComputeDensityGradient(bool v);	
			void setToComputeDensityHessian(bool v);	
			void setToComputeDensityLaplacian(bool v);	
			void setToComputeKEDensity(bool v);	
	};
	typedef shared_ptr<Properties> SharedProperties;
}
#endif