/* -*- mode: c++ -*-

  This file is part of the Feel library

  Author(s): Christophe Prud'homme <christophe.prudhomme@feelpp.org>
       Date: 2009-11-24

  Copyright (C) 2009 Universit� Joseph Fourier (Grenoble I)

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
/**
   \file crb_heatns.hpp
   \author Christophe Prud'homme <christophe.prudhomme@feelpp.org>
   \date 2009-11-24
 */
#ifndef __CRB_heatns_H
#define __CRB_heatns_H 1

#include <boost/multi_array.hpp>
#include <boost/tuple/tuple.hpp>
#include "boost/tuple/tuple_io.hpp"
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/bimap.hpp>
#include <boost/bimap/support/lambda.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <fstream>


#include <boost/serialization/vector.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/split_member.hpp>

#include <vector>

#include <Eigen/Core>
#include <Eigen/LU>
#include <Eigen/Dense>

#include <feel/feelalg/solvereigen.hpp>
#include <feel/feelcore/feel.hpp>
#include <feel/feelcore/environment.hpp>
#include <feel/feelcore/parameter.hpp>
#include <feel/feelcrb/parameterspace.hpp>
#include <feel/feelcrb/crbdb.hpp>
#include <feel/feelcrb/crbscm.hpp>
#include <feel/feelcore/serialization.hpp>
#include <feel/feelfilters/exporter.hpp>


namespace Feel
{
/**
 * CRBErrorType_heatns
 * Determine the type of error estimation used
 * - CRB_RESIDUAL : use the residual error estimation without algorithm SCM
 * - CRB_RESIDUAL_SCM : use the residual error estimation and also algorithm SCM
 * - CRB_NO_RESIDUAL : in this case we don't compute error estimation
 * - CRB_EMPIRICAL : compute |S_n - S_{n-1}| where S_n is the output obtained by using a reduced basis with n elements
 */
enum CRBErrorType_heatns { CRB_RESIDUAL = 0, CRB_RESIDUAL_SCM=1, CRB_NO_RESIDUAL=2 , CRB_EMPIRICAL=3};

/**
 * \class CRB
 * \brief Certifed Reduced Basis class
 *
 * Implements the certified reduced basis method
 *
 * @author Christophe Prud'homme
 * @see
 */
template<typename TruthModelType>
class CRB_heatns : public CRBDB
{
    typedef  CRBDB super;
public:


    /** @name Constants
     */
    //@{


    //@}

    /** @name Typedefs
     */
    //@{

    typedef TruthModelType truth_model_type;
    typedef truth_model_type model_type;
    typedef boost::shared_ptr<truth_model_type> truth_model_ptrtype;

    typedef double value_type;
    typedef boost::tuple<double,double> bounds_type;

    typedef ParameterSpace<TruthModelType::ParameterSpaceDimension> parameterspace_type;
    typedef boost::shared_ptr<parameterspace_type> parameterspace_ptrtype;
    typedef typename parameterspace_type::element_type parameter_type;
    typedef typename parameterspace_type::element_ptrtype parameter_ptrtype;
    typedef typename parameterspace_type::sampling_type sampling_type;
    typedef typename parameterspace_type::sampling_ptrtype sampling_ptrtype;

    typedef boost::tuple<double, parameter_type, size_type, double, double> relative_error_type;
    typedef relative_error_type max_error_type;

    typedef boost::tuple<double, std::vector< std::vector<double> > , std::vector< std::vector<double> >, double, double > error_estimation_type;
    typedef boost::tuple<double, std::vector<double> > residual_error_type;

    typedef boost::bimap< int, boost::tuple<double,double,double> > convergence_type;

    typedef typename convergence_type::value_type convergence;


    //! scm
    typedef CRBSCM<truth_model_type> scm_type;
    typedef boost::shared_ptr<scm_type> scm_ptrtype;


    //! function space type
    typedef typename model_type::functionspace_type functionspace_type;
    typedef typename model_type::functionspace_ptrtype functionspace_ptrtype;


    //! element of the functionspace type
    typedef typename model_type::element_type element_type;
    typedef typename model_type::element_ptrtype element_ptrtype;

    typedef typename model_type::backend_type backend_type;
    typedef boost::shared_ptr<backend_type> backend_ptrtype;
    typedef typename model_type::sparse_matrix_ptrtype sparse_matrix_ptrtype;
    typedef typename model_type::vector_ptrtype vector_ptrtype;
    typedef typename model_type::theta_vector_type theta_vector_type;


    typedef Eigen::VectorXd y_type;
    typedef std::vector<y_type> y_set_type;
    typedef std::vector<boost::tuple<double,double> > y_bounds_type;

    typedef std::vector<element_type> wn_type;
    typedef boost::tuple< std::vector<wn_type> , std::vector<std::string> > export_vector_wn_type;

    typedef std::vector<double> vector_double_type;
    typedef boost::shared_ptr<vector_double_type> vector_double_ptrtype;

    typedef Eigen::VectorXd vectorN_type;
    typedef Eigen::MatrixXd matrixN_type;



    typedef std::vector<element_type> mode_set_type;
    typedef boost::shared_ptr<mode_set_type> mode_set_ptrtype;

    typedef boost::multi_array<value_type, 2> array_2_type;
    typedef boost::multi_array<vectorN_type, 2> array_3_type;
    typedef boost::multi_array<matrixN_type, 2> array_4_type;

    //! mesh type
    typedef typename model_type::mesh_type mesh_type;
    typedef boost::shared_ptr<mesh_type> mesh_ptrtype;

    //! space type
    typedef typename model_type::space_type space_type;

    // ! export
    typedef Exporter<mesh_type> export_type;
    typedef boost::shared_ptr<export_type> export_ptrtype;



    //@}

    /** @name Constructors, destructor
     */
    //@{

    //! default constructor
    CRB_heatns()
        :
        super(),
        M_model(),
        M_output_index( 0 ),
        M_tolerance( 1e-2 ),
        M_iter_max( 3 ),
        M_factor( -1 ),
        M_error_type( CRB_NO_RESIDUAL ),
        M_Dmu( new parameterspace_type ),
        M_Xi( new sampling_type( M_Dmu ) ),
        M_WNmu( new sampling_type( M_Dmu, 1, M_Xi ) ),
        M_WNmu_complement(),
        exporter( Exporter<mesh_type>::New( "ensight" ) )
    {
    }

    //! constructor from command line options
    CRB_heatns( std::string  name,
         po::variables_map const& vm )
        :
        super( ( boost::format( "%1%" ) % vm["crb.error-type"].template as<int>() ).str(),
               name,
               ( boost::format( "%1%-%2%-%3%" ) % name % vm["crb.output-index"].template as<int>() % vm["crb.error-type"].template as<int>() ).str(),
               vm ),

        M_model(),
        M_backend( backend_type::build( vm ) ),
        M_output_index( vm["crb.output-index"].template as<int>() ),
        M_tolerance( vm["crb.error-max"].template as<double>() ),
        M_iter_max( vm["crb.dimension-max"].template as<int>() ),
        M_factor( vm["crb.factor"].template as<int>() ),
        M_error_type( CRBErrorType_heatns( vm["crb.error-type"].template as<int>() ) ),
        M_Dmu( new parameterspace_type ),
        M_Xi( new sampling_type( M_Dmu ) ),
        M_WNmu( new sampling_type( M_Dmu, 1, M_Xi ) ),
        M_WNmu_complement(),
        M_scmA( new scm_type( name+"_a", vm ) ),
        M_scmM( new scm_type( name+"_m", vm ) ),
        exporter( Exporter<mesh_type>::New( vm, "BasisFunction" ) )
    {
        if ( this->loadDB() )
            std::cout << "Database " << this->lookForDB() << " available and loaded\n";
    }

    //! constructor from command line options
    CRB_heatns( std::string  name,
         po::variables_map const& vm,
         truth_model_ptrtype const & model )
        :
        super( ( boost::format( "%1%" ) % vm["crb.error-type"].template as<int>() ).str(),
               name,
               ( boost::format( "%1%-%2%-%3%" ) % name % vm["crb.output-index"].template as<int>() % vm["crb.error-type"].template as<int>() ).str(),
               vm ),

        M_model(),
        M_backend( backend_type::build( vm ) ),
        M_output_index( vm["crb.output-index"].template as<int>() ),
        M_tolerance( vm["crb.error-max"].template as<double>() ),
        M_iter_max( vm["crb.dimension-max"].template as<int>() ),
        M_factor( vm["crb.factor"].template as<int>() ),
        M_error_type( CRBErrorType_heatns( vm["crb.error-type"].template as<int>() ) ),
        M_Dmu( new parameterspace_type ),
        M_Xi( new sampling_type( M_Dmu ) ),
        M_WNmu( new sampling_type( M_Dmu, 1, M_Xi ) ),
        M_WNmu_complement(),
        M_scmA( new scm_type( name+"_a", vm ) ),
        M_scmM( new scm_type( name+"_m", vm ) ),
        exporter( Exporter<mesh_type>::New( vm, "BasisFunction" ) )
    {
        this->setTruthModel( model );
        if ( this->loadDB() )
            std::cout << "Database " << this->lookForDB() << " available and loaded\n";

    }

    //! copy constructor
    CRB_heatns( CRB_heatns const & o )
        :
        super( o ),
        M_output_index( o.M_output_index ),
        M_tolerance( o.M_tolerance ),
        M_iter_max( o.M_iter_max ),
        M_factor( o.M_factor ),
        M_error_type( o.M_error_type ),
        M_Dmu( o.M_Dmu ),
        M_Xi( o.M_Xi ),
        M_WNmu( o.M_WNmu ),
        M_WNmu_complement( o.M_WNmu_complement ),
        M_C0_pr( o.M_C0_pr ),
        M_C0_du( o.M_C0_du ),
        M_Lambda_pr( o.M_Lambda_pr ),
        M_Lambda_du( o.M_Lambda_du ),
        M_Gamma_pr( o.M_Gamma_pr ),
        M_Gamma_du( o.M_Gamma_du ),
        M_Cmf_pr( o.M_Cmf_pr ),
        M_Cmf_du( o.M_Cmf_du ),
        M_Cma_pr( o.M_Cma_pr ),
        M_Cma_du( o.M_Cma_du ),
        M_Cmm_pr( o.M_Cmm_pr ),
        M_Cmm_du( o.M_Cmm_du ),
        M_coeff_pr_ini_online( o.M_coeff_pr_ini_online ),
        M_coeff_du_ini_online( o.M_coeff_du_ini_online )
    {}

    //! destructor
    ~CRB_heatns()
    {}


    //@}

    /** @name Operator overloads
     */
    //@{

    //! copy operator
    CRB_heatns& operator=( CRB_heatns const & o )
    {
        if ( this != &o )
        {
        }

        return *this;
    }
    //@}

    /** @name Accessors
     */
    //@{


    //! return factor
    int factor() const
    {
        return factor;
    }
    //! \return max iterations
    int maxIter() const
    {
        return M_iter_max;
    }

    //! \return the parameter space
    parameterspace_ptrtype Dmu() const
    {
        return M_Dmu;
    }

    //! \return the output index
    int outputIndex() const
    {
        return M_output_index;
    }

    //! \return the dimension of the reduced basis space
    int dimension() const
    {
        return M_N;
    }

    //! \return the train sampling used to generate the reduced basis space
    sampling_ptrtype trainSampling() const
    {
        return M_Xi;
    }

    //! \return the error type
    CRBErrorType_heatns errorType() const
    {
        return M_error_type;
    }

    //! \return the scm object
    scm_ptrtype scm() const
    {
        return M_scmA;
    }


    //@}

    /** @name  Mutators
     */
    //@{

    //! set the output index
    void setOutputIndex( uint16_type oindex )
    {
        int M_prev_o = M_output_index;
        M_output_index = oindex;

        if ( ( size_type )M_output_index >= M_model->Nl()  )
            M_output_index = M_prev_o;

        //std::cout << " -- crb set output index to " << M_output_index << " (max output = " << M_model->Nl() << ")\n";
        this->setDBFilename( ( boost::format( "%1%-%2%-%3%.crbdb" ) % this->name() % M_output_index % M_error_type ).str() );

        if ( M_output_index != M_prev_o )
            this->loadDB();

        //std::cout << "Database " << this->lookForDB() << " available and loaded\n";

    }

    //! set the crb error type
    void setCRBErrorType( CRBErrorType_heatns error )
    {
        M_error_type = error;
    }

    //! set offline tolerance
    void setTolerance( double tolerance )
    {
        M_tolerance = tolerance;
    }

    //! set the truth offline model
    void setTruthModel( truth_model_ptrtype const& model )
    {
        M_model = model;
        M_Dmu = M_model->parameterSpace();
        M_Xi = sampling_ptrtype( new sampling_type( M_Dmu ) );

        if ( ! loadDB() )
            M_WNmu = sampling_ptrtype( new sampling_type( M_Dmu ) );

        M_scmA->setTruthModel( M_model );
        M_scmM->setTruthModel( M_model );
    }

    //! set max iteration number
    void setMaxIter( int K )
    {
        M_iter_max = K;
    }

    //! set factor
    void setFactor( int Factor )
    {
        M_factor = Factor;
    }

    //@}

    /** @name  Methods
     */
    //@{

    /**
     * orthonormalize the basis
     */
    void orthonormalize( size_type N, wn_type& wn, int Nm = 1 );

    void checkResidual( parameter_type const& mu, std::vector< std::vector<double> > const& primal_residual_coeffs, std::vector< std::vector<double> > const& dual_residual_coeffs ) const;


    void buildFunctionFromRbCoefficients( std::vector< vectorN_type > const & RBcoeff, wn_type const & WN, std::vector<element_ptrtype> & FEMsolutions );

    /*
     * check orthonormality
     */
    //void checkOrthonormality( int N, const wn_type& wn) const;
    void checkOrthonormality( int N, const wn_type& wn ) const;

    /**
     * check the reduced basis space invariant properties
     * \param N dimension of \f$W_N\f$
     */
    void check( size_type N )  const;

    /**
     * compute effectivity indicator of the error estimation overall a given
     * parameter space
     *
     * \param max_ei : maximum efficiency indicator (output)
     * \param min_ei : minimum efficiency indicator (output)
     * \param Dmu (input) parameter space
     * \param N : sampling size (optional input with default value)
     */
    void computeErrorEstimationEfficiencyIndicator ( parameterspace_ptrtype const& Dmu, double& max_ei, double& min_ei,int N = 4 );


    /**
     * export basis functions to visualize it
     * \param wn : tuple composed of a vector of wn_type and a vector of string (used to name basis)
     */
    void exportBasisFunctions( const export_vector_wn_type& wn )const ;

    /**
     * Returns the lower bound of the output
     *
     * \param mu \f$ \mu\f$ the parameter at which to evaluate the output
     * \param N the size of the reduced basis space to use
     * \param uN primal solution
     * \param uNdu dual solution
     * \param K : index of time ( time = K*dt) at which we want to evaluate the output
     * Note : K as a default value for non time-dependent problems
     *
     *\return compute online the lower bound
     *\and also condition number of matrix A
     */

    //    boost::tuple<double,double> lb( size_type N, parameter_type const& mu, std::vector< vectorN_type >& uN, std::vector< vectorN_type >& uNdu , std::vector<vectorN_type> & uNold=std::vector<vectorN_type>(), std::vector<vectorN_type> & uNduold=std::vector<vectorN_type>(), int K=0) const;
    boost::tuple<double,double> lb( size_type N, parameter_type const& mu, std::vector< vectorN_type >& uN, std::vector< vectorN_type >& uNdu , std::vector<vectorN_type> & uNold, std::vector<vectorN_type> & uNduold, int K=0 ) const;


    /**
     * Returns the lower bound of the output
     *
     * \param mu \f$ \mu\f$ the parameter at which to evaluate the output
     * \param N the size of the reduced basis space to use
     * \param uN primal solution
     * \param uNdu dual solution
     *
     *\return compute online the lower bound and condition number of matrix A
     */
    boost::tuple<double,double> lb( parameter_ptrtype const& mu, size_type N, std::vector< vectorN_type >& uN, std::vector< vectorN_type >& uNdu ) const
    {
        return lb( N, *mu, uN, uNdu );
    }

    /**
     * Returns the upper bound of the output associed to \f$\mu\f$
     *
     * \param mu \f$ \mu\f$ the parameter at which to evaluate the output
     * \param N the dimension of \f$W_N\f$
     * \param uN primal solution
     * \param uNdu dual solution
     *
     *\return compute online the lower bound
     */


    value_type ub( size_type N, parameter_type const& mu, std::vector< vectorN_type >& uN, std::vector< vectorN_type >& uNdu ) const
    {
        auto o = lb( N, mu, uN, uNdu );
        auto e = delta( N, mu, uN, uNdu );
        return o.template get<0>() + e.template get<0>();
    }

    /**
     * Returns the error bound on the output
     *
     * \param mu \f$ \mu\f$ the parameter at which to evaluate the output
     * \param N the size of the reduced basis space to use
     * \param uN primal solution
     * \param uNdu dual solution
     *
     *\return compute online the lower bound
     */
    value_type delta( size_type N, parameter_ptrtype const& mu, std::vector< vectorN_type > const& uN, std::vector< vectorN_type > const& uNdu, std::vector<vectorN_type> const& uNold,  std::vector<vectorN_type> const& uNduold , int k=0 ) const
    {
        auto e = delta( N, mu, uN, uNdu );
        return e.template get<0>();
    }

    /**
     * Returns the error bound on the output associed to \f$\mu\f$
     *
     * \param mu \f$ \mu\f$ the parameter at which to evaluate the output
     * \param N the dimension of \f$W_N\f$
     * \param uN primal solution
     * \param uNdu dual solution
     *
     *\return compute online the lower bound
     */
    error_estimation_type delta( size_type N, parameter_type const& mu, std::vector< vectorN_type > const& uN, std::vector< vectorN_type > const& uNdu, std::vector<vectorN_type> const& uNold, std::vector<vectorN_type> const& uNduold, int k=0 ) const;

    /**
     * Returns the upper bound of the output associed to \f$\mu\f$
     *
     * \param mu \f$ \mu\f$ the parameter at which to evaluate the output
     * \param N the dimension of \f$W_N\f$
     *
     *\return compute online the lower bound
     */
    value_type ub( size_type K, parameter_ptrtype const& mu, std::vector< vectorN_type >& uN, std::vector< vectorN_type >& uNdu ) const
    {
        return ub( K, *mu, uN, uNdu );
    }

    /**
     * Offline computation
     *
     * \return the convergence history (max error)
     */
    convergence_type offline();


    /**
     * \brief Retuns maximum value of the relative error
     * \param N number of elements in the reduced basis <=> M_N
     */
    max_error_type maxErrorBounds( size_type N ) const;

    /**
     * evaluate online the residual
     */

    residual_error_type steadyPrimalResidual( int Ncur, parameter_type const& mu,  vectorN_type const& Un, double time=0 ) const;


    residual_error_type steadyDualResidual( int Ncur, parameter_type const& mu,  vectorN_type const& Un, double time=0 ) const;


    /**
     * generate offline the residual
     */
    void offlineResidual( int Ncur , int number_of_added_elements=1 );
    void offlineResidual( int Ncur, mpl::bool_<true> ,int number_of_added_elements=1 );
    void offlineResidual( int Ncur, mpl::bool_<false> , int number_of_added_elements=1 );

    /*
     * compute empirical error estimation, ie : |S_n - S{n-1}|
     * \param Ncur : number of elements in the reduced basis <=> M_N
     * \param mu : parameters value (one value per parameter)
     * \output : empirical error estimation
     */
    value_type empiricalErrorEstimation ( int Ncur, parameter_type const& mu, int k ) const ;


    /**
     * run the certified reduced basis with P parameters and returns 1 output
     */
    //boost::tuple<double,double,double> run( parameter_type const& mu, double eps = 1e-6 );
    boost::tuple<double,double,double,double> run( parameter_type const& mu, double eps = 1e-6 );

    /**
     * run the certified reduced basis with P parameters and returns 1 output
     */
    void run( const double * X, unsigned long N, double * Y, unsigned long P );

    /**
     * \return a random sampling
     */
    sampling_type randomSampling( int N )
    {
        M_Xi->randomize( N );
        return *M_Xi;
    }

    /**
     * \return a equidistributed sampling
     */
    sampling_type equidistributedSampling( int N )
    {
        M_Xi->equidistribute( N );
        return *M_Xi;
    }

    /**
     * save the CRB database
     */
    void saveDB();

    /**
     * load the CRB database
     */
    bool loadDB();

    /**
     * if true, rebuild the database (if already exist)
     */
    bool rebuildDB() ;

    /**
     * if true, show the mu selected during the offline stage
     */
    bool showMuSelection() ;

    /**
     * if true, print the max error (absolute) during the offline stage
     */
    bool printErrorDuringOfflineStep();

    /**
     * print parameters set mu selected during the offline stage
     */
    void printMuSelection( void );

    /**
     * print max errors (total error and also primal and dual contributions)
     * during offline stage
     */
    void printErrorsDuringRbConstruction( void );
    /*
     * compute correction terms for output
     * \param mu \f$ \mu\f$ the parameter at which to evaluate the output
     * \param uN : primal solution
     * \param uNdu : dual solution
     * \pram uNold : old primal solution
     * \param K : time index
     */
    double correctionTerms(parameter_type const& mu, std::vector< vectorN_type > const & uN, std::vector< vectorN_type > const & uNdu , std::vector<vectorN_type> const & uNold,  int const K=0) const;

    /*
     * build matrix to store functions used to compute the variance output
     * \param N : number of elements in the reduced basis
     */
    void buildVarianceMatrixPhi(int const N);
    //@}

private:

    truth_model_ptrtype M_model;

    backend_ptrtype M_backend;

    int M_output_index;

    double M_tolerance;

    size_type M_iter_max;

    int M_factor;

    CRBErrorType_heatns M_error_type;

    // parameter space
    parameterspace_ptrtype M_Dmu;

    // fine sampling of the parameter space
    sampling_ptrtype M_Xi;

    // sampling of parameter space to build WN
    sampling_ptrtype M_WNmu;
    sampling_ptrtype M_WNmu_complement;

    //scm
    scm_ptrtype M_scmA;
    scm_ptrtype M_scmM;

    //export
    export_ptrtype exporter;

    array_2_type M_C0_pr;
    array_2_type M_C0_du;
    array_3_type M_Lambda_pr;
    array_3_type M_Lambda_du;
    array_4_type M_Gamma_pr;
    array_4_type M_Gamma_du;

    array_3_type M_Cmf_pr;
    array_3_type M_Cmf_du;
    array_4_type M_Cma_pr;
    array_4_type M_Cma_du;
    array_4_type M_Cmm_pr;
    array_4_type M_Cmm_du;

    std::vector<double> M_coeff_pr_ini_online;
    std::vector<double> M_coeff_du_ini_online;


    friend class boost::serialization::access;
    // When the class Archive corresponds to an output archive, the
    // & operator is defined similar to <<.  Likewise, when the class Archive
    // is a type of input archive the & operator is defined similar to >>.
    template<class Archive>
    void save( Archive & ar, const unsigned int version ) const;

    template<class Archive>
    void load( Archive & ar, const unsigned int version ) ;

    BOOST_SERIALIZATION_SPLIT_MEMBER()

    // reduced basis space
    wn_type M_WN;
    wn_type M_WNdu;

    size_type M_N;
    size_type M_Nm;

    bool orthonormalize_primal;
    bool orthonormalize_dual;
    bool solve_dual_problem;

    convergence_type M_rbconv;

    // left hand side
    std::vector<matrixN_type> M_Aq_pr;
    std::vector<matrixN_type> M_Aq_du;
    std::vector<matrixN_type> M_Aq_pr_du;

    //mass matrix
    std::vector<matrixN_type> M_Mq_pr;
    std::vector<matrixN_type> M_Mq_du;
    std::vector<matrixN_type> M_Mq_pr_du;

    // right hand side
    std::vector<vectorN_type> M_Fq_pr;
    std::vector<vectorN_type> M_Fq_du;
    // output
    std::vector<vectorN_type> M_Lq_pr;
    std::vector<vectorN_type> M_Lq_du;

    std::vector<int> M_index;
    int M_mode_number;

    matrixN_type M_variance_matrix_phi;

    bool M_compute_variance;
    bool M_rbconv_contains_primal_and_dual_contributions;
    parameter_type M_current_mu;
};

po::options_description crbOptions( std::string const& prefix = "" );


template<typename TruthModelType>
typename CRB_heatns<TruthModelType>::convergence_type
CRB_heatns<TruthModelType>::offline()
{
    std::cout << "-------->CRB_heatns<TruthModelType>::offline()\n";

    M_rbconv_contains_primal_and_dual_contributions = true;

    bool rebuild_database = this->vm()["crb.rebuild-database"].template as<bool>() ;
    orthonormalize_primal = this->vm()["crb.orthonormalize-primal"].template as<bool>() ;
    orthonormalize_dual = this->vm()["crb.orthonormalize-dual"].template as<bool>() ;
    solve_dual_problem = this->vm()["crb.solve-dual-problem"].template as<bool>() ;
    if ( ! solve_dual_problem ) orthonormalize_dual=false;

    std::cout << "-------->orthonormalize_primal = " << orthonormalize_primal << std::endl;
    std::cout << "-------->solve_dual_problem = " << solve_dual_problem << std::endl;
    int nn = this->vm()["crb.dimension-max"].template as<int>() ;
    std::cout << "-------->nn = " << nn << std::endl;



    M_Nm = this->vm()["crb.Nm"].template as<int>() ;
    bool seek_mu_in_complement = this->vm()["crb.seek-mu-in-complement"].template as<bool>() ;

    boost::timer ti;
    std::cout << "Offline CRB starts, this may take a while until Database is computed...\n";
    LOG(INFO) << "[CRB::offline] initialize underlying finite element model\n";
    M_model->init();
    std::cout << " -- model init done in " << ti.elapsed() << "s\n";

    parameter_type mu( M_Dmu );

    double maxerror;
    double delta_pr;
    double delta_du;
    size_type index;

    //if M_N == 0 then there is not an already existing database
    if ( rebuild_database || M_N == 0)
    {
        ti.restart();
        //scm_ptrtype M_scm = scm_ptrtype( new scm_type( M_vm ) );
        //M_scm->setTruthModel( M_model );
        //    std::vector<boost::tuple<double,double,double> > M_rbconv2 = M_scm->offline();

        LOG(INFO) << "[CRB::offline] compute random sampling\n";
        // random sampling
        M_Xi->randomize( this->vm()["crb.sampling-size"].template as<int>() );
        //M_Xi->equidistribute( this->vm()["crb.sampling-size"].template as<int>() );
        M_WNmu->setSuperSampling( M_Xi );

        std::cout<<"[CRB offline] M_error_type = "<<M_error_type<<std::endl;
        std::cout << " -- sampling init done in " << ti.elapsed() << "s\n";

        LOG(INFO) << "[CRB::offline] Starting offline for output " << M_output_index << "\n";
        ti.restart();

        if ( M_error_type == CRB_RESIDUAL || M_error_type == CRB_RESIDUAL_SCM )
        {
            int __QLhs = M_model->Qa();
            int __QRhs = M_model->Ql( 0 );
            int __QOutput = M_model->Ql( M_output_index );
            int __Qm = M_model->Qm();

            typename array_2_type::extent_gen extents2;
            M_C0_pr.resize( extents2[__QRhs][__QRhs] );
            M_C0_du.resize( extents2[__QOutput][__QOutput] );

            typename array_3_type::extent_gen extents3;
            M_Lambda_pr.resize( extents3[__QLhs][__QRhs] );
            M_Lambda_du.resize( extents3[__QLhs][__QOutput] );

            typename array_4_type::extent_gen extents4;
            M_Gamma_pr.resize( extents4[__QLhs][__QLhs] );
            M_Gamma_du.resize( extents4[__QLhs][__QLhs] );

        }
        std::cout << " -- residual data init done in " << ti.elapsed() << "\n";
        ti.restart();

        // empty sets
        M_WNmu->clear();

        // start with M_C = { arg min mu, mu \in Xi }
        boost::tie( mu, index ) = M_Xi->min();

        std::cout << "  -- start with mu = [ ";
        int size = mu.size();
        for ( int i=0; i<size-1; i++ ) std::cout<<mu( i )<<" ";
        std::cout<<mu( size-1 )<<" ]"<<std::endl;
        //std::cout << " -- WN size :  " << M_WNmu->size() << "\n";

        // dimension of reduced basis space
        M_N = 0;

        // scm offline stage: build C_K
        if ( M_error_type == CRB_RESIDUAL_SCM )
        {
            M_scmA->setScmForMassMatrix( false );
            std::vector<boost::tuple<double,double,double> > M_rbconv2 = M_scmA->offline();

            if ( ! M_model->isSteady() )
            {
                M_scmM->setScmForMassMatrix( true );
                std::vector<boost::tuple<double,double,double> > M_rbconv3 = M_scmM->offline();
            }
        }

        maxerror = 1e10;
        delta_pr = 0;
        delta_du = 0;
        //boost::tie( maxerror, mu, index ) = maxErrorBounds( N );

        LOG(INFO) << "[CRB::offline] allocate reduced basis data structures\n";
        M_Aq_pr.resize( M_model->Qa() );

        M_Aq_du.resize( M_model->Qa() );
        M_Aq_pr_du.resize( M_model->Qa() );
        M_Mq_pr.resize( M_model->Qm() );
        M_Mq_du.resize( M_model->Qm() );
        M_Mq_pr_du.resize( M_model->Qm() );

        M_Fq_pr.resize( M_model->Ql( 0 ) );
        M_Fq_du.resize( M_model->Ql( 0 ) );
        M_Lq_pr.resize( M_model->Ql( M_output_index ) );
        M_Lq_du.resize( M_model->Ql( M_output_index ) );

    }//end of if( rebuild_database )
    else
    {
        mu = M_current_mu;
        std::cout<<"we are going to enrich the reduced basis"<<std::endl;
        std::cout<<"there are "<<M_N<<" elements in the database"<<std::endl;
    }//end of else associated to if ( rebuild_databse )

    sparse_matrix_ptrtype M,A,Adu,At;
    std::vector<vector_ptrtype> F,L;

    LOG(INFO) << "[CRB::offline] compute affine decomposition\n";
    std::vector<sparse_matrix_ptrtype> Aq;
    std::vector<sparse_matrix_ptrtype> Mq;
    std::vector<std::vector<vector_ptrtype> > Fq,Lq;
    sparse_matrix_ptrtype Aq_transpose = M_model->newMatrix();
    boost::tie( Mq, Aq, Fq ) = M_model->computeAffineDecomposition();

    element_ptrtype u( new element_type( M_model->functionSpace() ) );
    element_ptrtype uproj( new element_type( M_model->functionSpace() ) );
    element_ptrtype udu( new element_type( M_model->functionSpace() ) );
    element_ptrtype dual_initial_field( new element_type( M_model->functionSpace() ) );
    vector_ptrtype Rhs( M_backend->newVector( M_model->functionSpace() ) );

    M_mode_number=1;

    LOG(INFO) << "[CRB::offline] starting offline adaptive loop\n";

    bool reuse_prec = this->vm()["crb.reuse-prec"].template as<bool>() ;

    int no_residual_index = 0;
    sampling_ptrtype Sampling;
    int sampling_size=no_residual_index+1;

    if ( M_error_type == CRB_NO_RESIDUAL )
    {

        if( ! rebuild_database )
            throw std::logic_error( "[CRB::offline] ERROR , with crb.error-type=2 it's not possible to enrich an already existing reduced basis, please set crb.rebuild-database=true" );

        Sampling = sampling_ptrtype( new sampling_type( M_Dmu ) );
        sampling_size = M_iter_max / M_Nm ;

        if ( M_iter_max < M_Nm ) sampling_size = 1;

        Sampling->logEquidistribute( sampling_size );
        LOG(INFO) << "[CRB::offline] sampling parameter space with "<< sampling_size <<"elements done\n";
    }

    LOG(INFO) << "[CRB::offline] strategy "<< M_error_type <<"\n";
    std::cout << "[CRB::offline] strategy "<< M_error_type <<"\n";

    while ( maxerror > M_tolerance && M_N < M_iter_max && no_residual_index<sampling_size )
    {

        if ( M_error_type == CRB_NO_RESIDUAL )  mu = M_Xi->at( no_residual_index );

        boost::timer timer, timer2;
        LOG(INFO) <<"========================================"<<"\n";
        std::cout << "============================================================\n";

        if ( M_error_type == CRB_NO_RESIDUAL )
        {
            std::cout << "N=" << M_N << "/"  << M_iter_max <<"\n";
            LOG(INFO) << "N=" << M_N << "/"  << M_iter_max << "\n";
        }

        else
        {
            std::cout << "N=" << M_N << "/"  << M_iter_max << " maxerror=" << maxerror << " / "  << M_tolerance << "\n";
            LOG(INFO) << "N=" << M_N << "/"  << M_iter_max << " maxerror=" << maxerror << " / "  << M_tolerance << "\n";
        }


        backend_ptrtype backend_primal_problem = backend_type::build( BACKEND_PETSC );
        backend_ptrtype backend_dual_problem = backend_type::build( BACKEND_PETSC );


        // for a given parameter \p mu assemble the left and right hand side
        u->setName( ( boost::format( "fem-primal-%1%" ) % ( M_N ) ).str() );
        udu->setName( ( boost::format( "fem-dual-%1%" ) % ( M_N ) ).str() );

        if ( M_model->isSteady() || !model_type::is_time_dependent )
        {
            u->zero();
            udu->zero();
            boost::tie( M, A, F ) = M_model->update( mu );

            std::cout << "  -- updated model for parameter in " << timer2.elapsed() << "s\n";
            timer2.restart();
            LOG(INFO) << "[CRB::offline] transpose primal matrix" << "\n";

            At = M_model->newMatrix();
            A->transpose( At );

            //u->setName( ( boost::format( "fem-primal-%1%" ) % ( M_N ) ).str() );
            //udu->setName( ( boost::format( "fem-dual-%1%" ) % ( M_N ) ).str() );

            LOG(INFO) << "[CRB::offline] solving primal" << "\n";
            backend_primal_problem->solve( _matrix=A,  _solution=u, _rhs=F[0] );

#if 0
            std::ofstream file_solution;
            std::string mu_str;

            for ( int i=0; i<mu.size(); i++ )
            {
                mu_str= mu_str + ( boost::format( "_%1%" ) %mu[i] ).str() ;
            }

            std::string name = "CRBsolution" + mu_str;
            file_solution.open( name,std::ios::out );

            for ( int i=0; i<u->size(); i++ ) file_solution<<u->operator()( i )<<"\n";

            file_solution.close();

            name = "CRBrhs" + mu_str;
            std::ofstream file_rhs;
            file_rhs.open( name,std::ios::out );
            file_rhs<<*F[0];
            file_rhs.close();

#endif
#if 0
            std::ofstream file_matrix;
            name = "CRBmatrix" + mu_str;
            file_matrix.open( name,std::ios::out );
            file_matrix<<*A;
            file_matrix.close();
#endif


            //std::cout << "solving primal done" << std::endl;
            std::cout << "  -- primal problem solved in " << timer2.elapsed() << "s\n";
            timer2.restart();
            *Rhs = *F[M_output_index];
            Rhs->scale( -1 );
            backend_dual_problem->solve( _matrix=At,  _solution=udu, _rhs=Rhs );
            std::cout<<"dual solved "<<std::endl;
        }
        else
        {
            std::cout << "ERROR : I'm not transient !!!" << std::endl;
        }

        for ( size_type l = 0; l < M_model->Nl(); ++l )
            LOG(INFO) << "u^T F[" << l << "]= " << inner_product( *u, *F[l] ) << "\n";

        LOG(INFO) << "[CRB::offline] energy = " << A->energy( *u, *u ) << "\n";

        M_WNmu->push_back( mu, index );
        M_WNmu_complement = M_WNmu->complement();

        bool norm_zero = false;

        if ( M_model->isSteady() )
        {
            M_WN.push_back( *u );
            M_WNdu.push_back( *udu );

        }//end of steady case

        else
        {
            std::cout << "ERROR : I'm not transient !!!" << std::endl;
        }//end of transient case

        size_type number_of_added_elements;

        if( M_model->isSteady() )
            number_of_added_elements=1;

        std::cout<<"-------->number_of_added_elements = "<<number_of_added_elements<<std::endl;

        M_N+=number_of_added_elements;

        if ( orthonormalize_primal )
        {
            orthonormalize( M_N, M_WN, number_of_added_elements );
            orthonormalize( M_N, M_WN, number_of_added_elements );
            orthonormalize( M_N, M_WN, number_of_added_elements );
        }

        if ( orthonormalize_dual )
        {
            orthonormalize( M_N, M_WNdu, number_of_added_elements );
            orthonormalize( M_N, M_WNdu, number_of_added_elements );
            orthonormalize( M_N, M_WNdu, number_of_added_elements );
        }


        LOG(INFO) << "[CRB::offline] compute Aq_pr, Aq_du, Aq_pr_du" << "\n";

        for ( size_type q = 0; q < M_model->Qa(); ++q )
        {
            M_Aq_pr[q].conservativeResize( M_N, M_N );
            M_Aq_du[q].conservativeResize( M_N, M_N );
            M_Aq_pr_du[q].conservativeResize( M_N, M_N );

            // only compute the last line and last column of reduced matrices
            for ( size_type i = M_N-number_of_added_elements; i < M_N; i++ )
            {
                for ( size_type j = 0; j < M_N; ++j )
                {
                    M_Aq_pr[q]( i, j ) = Aq[q]->energy( M_WN[i], M_WN[j] );
                    M_Aq_du[q]( i, j ) = Aq[q]->energy( M_WNdu[i], M_WNdu[j], true );
                    M_Aq_pr_du[q]( i, j ) = Aq[q]->energy( M_WNdu[i], M_WN[j] );
                }
            }

            for ( size_type j=M_N-number_of_added_elements; j < M_N; j++ )
            {
                for ( size_type i = 0; i < M_N; ++i )
                {
                    M_Aq_pr[q]( i, j ) = Aq[q]->energy( M_WN[i], M_WN[j] );
                    M_Aq_du[q]( i, j ) = Aq[q]->energy( M_WNdu[i], M_WNdu[j], true );
                    M_Aq_pr_du[q]( i, j ) = Aq[q]->energy( M_WNdu[i], M_WN[j] );
                }
            }
        }


        LOG(INFO) << "[CRB::offline] compute Mq_pr, Mq_du, Mq_pr_du" << "\n";

        for ( size_type q = 0; q < M_model->Qm(); ++q )
        {
            M_Mq_pr[q].conservativeResize( M_N, M_N );
            M_Mq_du[q].conservativeResize( M_N, M_N );
            M_Mq_pr_du[q].conservativeResize( M_N, M_N );

            // only compute the last line and last column of reduced matrices
            for ( size_type i=M_N-number_of_added_elements ; i < M_N; i++ )
            {
                for ( size_type j = 0; j < M_N; ++j )
                {
                    M_Mq_pr[q]( i, j ) = Mq[q]->energy( M_WN[i], M_WN[j] );
                    M_Mq_du[q]( i, j ) = Mq[q]->energy( M_WNdu[i], M_WNdu[j], true );
                    M_Mq_pr_du[q]( i, j ) = Mq[q]->energy( M_WNdu[i], M_WN[j] );
                }
            }

            for ( size_type j = M_N-number_of_added_elements; j < M_N ; j++ )
            {
                for ( size_type i = 0; i < M_N; ++i )
                {
                    M_Mq_pr[q]( i, j ) = Mq[q]->energy( M_WN[i], M_WN[j] );
                    M_Mq_du[q]( i, j ) = Mq[q]->energy( M_WNdu[i], M_WNdu[j], true );
                    M_Mq_pr_du[q]( i, j ) = Mq[q]->energy( M_WNdu[i], M_WN[j] );
                }
            }
        }

        LOG(INFO) << "[CRB::offline] compute Fq_pr, Fq_du" << "\n";

        for ( size_type q = 0; q < M_model->Ql( 0 ); ++q )
        {
            M_Fq_pr[q].conservativeResize( M_N );
            M_Fq_du[q].conservativeResize( M_N );

            for ( size_type l = 1; l <= number_of_added_elements; ++l )
            {
                int index = M_N-l;
                M_Fq_pr[q]( index ) = M_model->Fq( 0, q, M_WN[index] );
                M_Fq_du[q]( index ) = M_model->Fq( 0, q, M_WNdu[index] );
            }
        }

        LOG(INFO) << "[CRB::offline] compute Lq_pr, Lq_du" << "\n";

        for ( size_type q = 0; q < M_model->Ql( M_output_index ); ++q )
        {
            M_Lq_pr[q].conservativeResize( M_N );
            M_Lq_du[q].conservativeResize( M_N );

            for ( size_type l = 1; l <= number_of_added_elements; ++l )
            {
                int index = M_N-l;
                M_Lq_pr[q]( index ) = M_model->Fq( M_output_index, q, M_WN[index] );
                M_Lq_du[q]( index ) = M_model->Fq( M_output_index, q, M_WNdu[index] );
            }

        }

        LOG(INFO) << "compute coefficients needed for the initialization of unknown in the online step\n";

        element_ptrtype primal_initial_field ( new element_type ( M_model->functionSpace() ) );
        element_ptrtype projection    ( new element_type ( M_model->functionSpace() ) );
        M_model->initializationField( primal_initial_field, mu ); //fill initial_field


        if ( model_type::is_time_dependent || !M_model->isSteady() )
        {
            std::cout << "ERROR : I'm not transient !!!" << std::endl;
        }


        timer2.restart();

        M_compute_variance = this->vm()["crb.compute-variance"].template as<bool>();
        buildVarianceMatrixPhi( M_N );

        if ( M_error_type==CRB_RESIDUAL || M_error_type == CRB_RESIDUAL_SCM )
        {
            std::cout << "  -- offlineResidual update starts\n";
            offlineResidual( M_N, number_of_added_elements );
            LOG(INFO)<<"[CRB::offline] end of call offlineResidual and M_N = "<< M_N <<"\n";
            std::cout << "  -- offlineResidual updated in " << timer2.elapsed() << "s\n";
            timer2.restart();
        }


        if ( M_error_type == CRB_NO_RESIDUAL )
        {
            maxerror=M_iter_max-M_N;
            no_residual_index++;
        }

        else
        {
            boost::tie( maxerror, mu, index , delta_pr , delta_du ) = maxErrorBounds( M_N );
            M_index.push_back( index );
            M_current_mu = mu;

            int count = std::count( M_index.begin(),M_index.end(),index );
            M_mode_number = count;

            std::cout << "  -- max error bounds computed in " << timer2.elapsed() << "s\n";
            timer2.restart();
        }

        M_rbconv.insert( convergence( M_N, boost::make_tuple(maxerror,delta_pr,delta_du) ) );

        //mu = M_Xi->at( M_N );//M_WNmu_complement->min().template get<0>();

        check( M_WNmu->size() );


        if ( this->vm()["crb.check.rb"].template as<int>() == 1 )std::cout << "  -- check reduced basis done in " << timer2.elapsed() << "s\n";

        timer2.restart();
        std::cout << "time: " << timer.elapsed() << "\n";
        LOG(INFO) << "time: " << timer.elapsed() << "\n";
        std::cout << "============================================================\n";
        LOG(INFO) <<"========================================"<<"\n";

        //save DB after adding an element
        this->saveDB();

    }

    std::cout << "-------->\n";
    std::cout<<"number of elements in the reduced basis : "<<M_N<<std::endl;
    LOG(INFO) << " index choosen : ";
    BOOST_FOREACH( auto id, M_index )
    LOG(INFO)<<id<<" ";
    LOG(INFO)<<"\n";
    bool visualize_basis = this->vm()["crb.visualize-basis"].template as<bool>() ;

    if ( visualize_basis )
    {
        std::vector<wn_type> wn;
        std::vector<std::string> names;
        wn.push_back( M_WN );
        names.push_back( "primal" );
        wn.push_back( M_WNdu );
        names.push_back( "dual" );
        exportBasisFunctions( boost::make_tuple( wn ,names ) );

        if ( orthonormalize_primal || orthonormalize_dual )
        {
            std::cout<<"[CRB::offline] Basis functions have been exported but warning elements have been orthonormalized"<<std::endl;
        }
    }


    //this->saveDB();
    std::cout << "Offline CRB is done\n";

    return M_rbconv;

}




template<typename TruthModelType>
void
CRB_heatns<TruthModelType>::buildVarianceMatrixPhi( int const N )
{
}



template<typename TruthModelType>
void
CRB_heatns<TruthModelType>::buildFunctionFromRbCoefficients( std::vector< vectorN_type > const & RBcoeff, wn_type const & WN, std::vector<element_ptrtype> & FEMsolutions )
{

    if( WN.size() == 0 )
        throw std::logic_error( "[CRB::buildFunctionFromRbCoefficients] ERROR : reduced basis space is empty" );


    int nb_solutions = RBcoeff.size();

    for( int i = 0; i < nb_solutions; i++ )
    {
        element_ptrtype FEMelement ( new element_type( M_model->functionSpace() ) );
        FEMelement->setZero();
        for( int j = 0; j < WN.size(); j++ )
            FEMelement->add( RBcoeff[i](j) , WN[j] );
        FEMsolutions.push_back( FEMelement );
    }
}


template<typename TruthModelType>
void
CRB_heatns<TruthModelType>::checkResidual( parameter_type const& mu, std::vector< std::vector<double> > const& primal_residual_coeffs, std::vector< std::vector<double> > const& dual_residual_coeffs  ) const
{


    if ( orthonormalize_primal || orthonormalize_dual )
    {
        throw std::logic_error( "[CRB::checkResidual] ERROR : to check residual don't use orthonormalization" );
    }

    if ( !M_model->isSteady() )
    {
        std::cout << "ERROR : I'm not transient !!!" << std::endl;
    }

    if ( M_error_type==CRB_NO_RESIDUAL || M_error_type==CRB_EMPIRICAL )
    {
        throw std::logic_error( "[CRB::checkResidual] ERROR : to check residual set option crb.error-type to 0 or 1" );
    }

    int size = mu.size();

    if ( 0 )
    {
        std::cout<<"[CRB::checkResidual] use mu = [";

        for ( int i=0; i<size-1; i++ ) std::cout<< mu[i] <<" , ";

        std::cout<< mu[size-1]<<" ]"<<std::endl;
    }

    sparse_matrix_ptrtype A,At,M;
    std::vector<vector_ptrtype> F,L;

    backend_ptrtype backendA = backend_type::build( BACKEND_PETSC );
    backend_ptrtype backendAt = backend_type::build( BACKEND_PETSC );

    element_ptrtype u( new element_type( M_model->functionSpace() ) );
    element_ptrtype udu( new element_type( M_model->functionSpace() ) );
    vector_ptrtype U( M_backend->newVector( M_model->functionSpace() ) );
    vector_ptrtype Rhs( M_backend->newVector( M_model->functionSpace() ) );

    boost::timer timer, timer2;

    boost::tie( boost::tuples::ignore, A, F ) = M_model->update( mu );

    LOG(INFO) << "  -- updated model for parameter in " << timer2.elapsed() << "s\n";
    timer2.restart();

    LOG(INFO) << "[CRB::checkResidual] transpose primal matrix" << "\n";
    At = M_model->newMatrix();
    A->transpose( At );
    u->setName( ( boost::format( "fem-primal-%1%" ) % ( M_N ) ).str() );
    udu->setName( ( boost::format( "fem-dual-%1%" ) % ( M_N ) ).str() );

    LOG(INFO) << "[CRB::checkResidual] solving primal" << "\n";
    backendA->solve( _matrix=A,  _solution=u, _rhs=F[0] );
    LOG(INFO) << "  -- primal problem solved in " << timer2.elapsed() << "s\n";
    timer2.restart();
    *Rhs = *F[M_output_index];
    Rhs->scale( -1 );
    backendAt->solve( _matrix=At,  _solution=udu, _rhs=Rhs );
    LOG(INFO) << "  -- dual problem solved in " << timer2.elapsed() << "s\n";
    timer2.restart();


    vector_ptrtype Aun( M_backend->newVector( M_model->functionSpace() ) );
    vector_ptrtype Atun( M_backend->newVector( M_model->functionSpace() ) );
    vector_ptrtype Un( M_backend->newVector( M_model->functionSpace() ) );
    vector_ptrtype Undu( M_backend->newVector( M_model->functionSpace() ) );
    vector_ptrtype Frhs( M_backend->newVector( M_model->functionSpace() ) );
    vector_ptrtype Lrhs( M_backend->newVector( M_model->functionSpace() ) );
    *Un = *u;
    *Undu = *udu;
    A->multVector( Un, Aun );
    At->multVector( Undu, Atun );
    Aun->scale( -1 );
    Atun->scale( -1 );
    *Frhs = *F[0];
    *Lrhs = *F[M_output_index];
    LOG(INFO) << "[CRB::checkResidual] residual (f,f) " << M_N-1 << ":=" << M_model->scalarProduct( Frhs, Frhs ) << "\n";
    LOG(INFO) << "[CRB::checkResidual] residual (f,A) " << M_N-1 << ":=" << 2*M_model->scalarProduct( Frhs, Aun ) << "\n";
    LOG(INFO) << "[CRB::checkResidual] residual (A,A) " << M_N-1 << ":=" << M_model->scalarProduct( Aun, Aun ) << "\n";

    LOG(INFO) << "[CRB::checkResidual] residual (l,l) " << M_N-1 << ":=" << M_model->scalarProduct( Lrhs, Lrhs ) << "\n";
    LOG(INFO) << "[CRB::checkResidual] residual (l,At) " << M_N-1 << ":=" << 2*M_model->scalarProduct( Lrhs, Atun ) << "\n";
    LOG(INFO) << "[CRB::checkResidual] residual (At,At) " << M_N-1 << ":=" << M_model->scalarProduct( Atun, Atun ) << "\n";


    Lrhs->scale( -1 );


    vector_ptrtype __ef_pr(  M_backend->newVector( M_model->functionSpace() ) );
    vector_ptrtype __ea_pr(  M_backend->newVector( M_model->functionSpace() ) );
    M_model->l2solve( __ef_pr, Frhs );
    M_model->l2solve( __ea_pr, Aun );
    double check_C0_pr = M_model->scalarProduct( __ef_pr,__ef_pr );
    double check_Lambda_pr = 2*M_model->scalarProduct( __ef_pr,__ea_pr );
    double check_Gamma_pr = M_model->scalarProduct( __ea_pr,__ea_pr );

    vector_ptrtype __ef_du(  M_backend->newVector( M_model->functionSpace() ) );
    vector_ptrtype __ea_du(  M_backend->newVector( M_model->functionSpace() ) );
    M_model->l2solve( __ef_du, Lrhs );
    M_model->l2solve( __ea_du, Atun );
    double check_C0_du = M_model->scalarProduct( __ef_du,__ef_du );
    double check_Lambda_du = 2*M_model->scalarProduct( __ef_du,__ea_du );
    double check_Gamma_du = M_model->scalarProduct( __ea_du,__ea_du );

    double primal_sum =  check_C0_pr + check_Lambda_pr + check_Gamma_pr ;
    double dual_sum   =  check_C0_du + check_Lambda_du + check_Gamma_du ;

    LOG(INFO)<<"[CRB::checkResidual] primal_sum = "<<check_C0_pr<<" + "<<check_Lambda_pr<<" + "<<check_Gamma_pr<<" = "<<primal_sum<<"\n";
    LOG(INFO)<<"[CRB::checkResidual] dual_sum = "<<check_C0_du<<" + "<<check_Lambda_du<<" + "<<check_Gamma_du<<" = "<<dual_sum<<"\n";


    int time_index=0;
    double C0_pr = primal_residual_coeffs[time_index][0];
    double Lambda_pr = primal_residual_coeffs[time_index][1];
    double Gamma_pr = primal_residual_coeffs[time_index][2];

    double C0_du,Lambda_du,Gamma_du;
    C0_du = dual_residual_coeffs[time_index][0];
    Lambda_du = dual_residual_coeffs[time_index][1];
    Gamma_du = dual_residual_coeffs[time_index][2];

    double err_C0_pr = math::abs( C0_pr - check_C0_pr ) ;
    double err_Lambda_pr = math::abs( Lambda_pr - check_Lambda_pr ) ;
    double err_Gamma_pr = math::abs( Gamma_pr - check_Gamma_pr ) ;
    double err_C0_du = math::abs( C0_du - check_C0_du ) ;
    double err_Lambda_du = math::abs( Lambda_du - check_Lambda_du ) ;
    double err_Gamma_du = math::abs( Gamma_du - check_Gamma_du ) ;

    int start_dual_index = 6;

    std::cout<<"[CRB::checkResidual]"<<std::endl;
    std::cout<<"====primal coefficients==== "<<std::endl;
    std::cout<<"              c0_pr \t\t lambda_pr \t\t gamma_pr"<<std::endl;
    std::cout<<"computed : ";

    for ( int i=0; i<3; i++ ) std::cout<<std::setprecision( 16 )<<primal_residual_coeffs[time_index][i]<<"\t";

    std::cout<<"\n";
    std::cout<<"true     : ";
    std::cout<<std::setprecision( 16 )<<check_C0_pr<<"\t"<<check_Lambda_pr<<"\t"<<check_Gamma_pr<<"\n"<<std::endl;
    std::cout<<"====dual coefficients==== "<<std::endl;
    std::cout<<"              c0_du \t\t lambda_du \t\t gamma_du"<<std::endl;
    std::cout<<"computed : ";

    for ( int i=0; i<3; i++ ) std::cout<<std::setprecision( 16 )<<dual_residual_coeffs[time_index][i]<<"\t";

    std::cout<<"\ntrue     : ";
    std::cout<<std::setprecision( 16 )<<check_C0_du<<"\t"<<check_Lambda_du<<"\t"<<check_Gamma_du<<"\n"<<std::endl;
    std::cout<<std::setprecision( 16 )<<"primal_true_sum = "<<check_C0_pr+check_Lambda_pr+check_Gamma_pr<<std::endl;
    std::cout<<std::setprecision( 16 )<<"dual_true_sum = "<<check_C0_du+check_Lambda_du+check_Gamma_du<<"\n"<<std::endl;

    std::cout<<std::setprecision( 16 )<<"primal_computed_sum = "<<C0_pr+Lambda_pr+Gamma_pr<<std::endl;
    std::cout<<std::setprecision( 16 )<<"dual_computed_sum = "<<C0_du+Lambda_du+Gamma_du<<"\n"<<std::endl;

    std::cout<<"errors committed on coefficients "<<std::endl;
    std::cout<<std::setprecision( 16 )<<"C0_pr : "<<err_C0_pr<<"\tLambda_pr : "<<err_Lambda_pr<<"\tGamma_pr : "<<err_Gamma_pr<<std::endl;
    std::cout<<std::setprecision( 16 )<<"C0_du : "<<err_C0_du<<"\tLambda_du : "<<err_Lambda_du<<"\tGamma_du : "<<err_Gamma_du<<std::endl;
    std::cout<<"and now relative error : "<<std::endl;
    double errC0pr = err_C0_pr/check_C0_pr;
    double errLambdapr = err_Lambda_pr/check_Lambda_pr;
    double errGammapr = err_Gamma_pr/check_Gamma_pr;
    double errC0du = err_C0_du/check_C0_du;
    double errLambdadu = err_Lambda_pr/check_Lambda_pr;
    double errGammadu = err_Gamma_pr/check_Gamma_pr;
    std::cout<<std::setprecision( 16 )<<errC0pr<<"\t"<<errLambdapr<<"\t"<<errGammapr<<std::endl;
    std::cout<<std::setprecision( 16 )<<errC0du<<"\t"<<errLambdadu<<"\t"<<errGammadu<<std::endl;

    //residual r(v)
    Aun->add( *Frhs );
    //Lrhs->scale( -1 );
    Atun->add( *Lrhs );
    //to have dual norm of residual we need to solve ( e , v ) = r(v) and then it's given by ||e||
    vector_ptrtype __e_pr(  M_backend->newVector( M_model->functionSpace() ) );
    M_model->l2solve( __e_pr, Aun );
    double dual_norm_pr = math::sqrt ( M_model->scalarProduct( __e_pr,__e_pr ) );
    std::cout<<"[CRB::checkResidual] dual norm of primal residual without isolate terms (c0,lambda,gamma) = "<<dual_norm_pr<<"\n";
    //idem for the dual equation
    vector_ptrtype __e_du(  M_backend->newVector( M_model->functionSpace() ) );
    M_model->l2solve( __e_du, Atun );
    double dual_norm_du = math::sqrt ( M_model->scalarProduct( __e_du,__e_du ) );
    std::cout <<"[CRB::checkResidual] dual norm of dual residual without isolate terms = "<<dual_norm_du<<"\n";

    double err_primal = math::sqrt ( M_model->scalarProduct( Aun, Aun ) );
    double err_dual = math::sqrt ( M_model->scalarProduct( Atun, Atun ) );
    LOG(INFO) << "[CRB::checkResidual] true primal residual for reduced basis function " << M_N-1 << ":=" << err_primal << "\n";
    LOG(INFO) << "[CRB::checkResidual] true dual residual for reduced basis function " << M_N-1 << ":=" << err_dual << "\n";

}


template<typename TruthModelType>
void
CRB_heatns<TruthModelType>::check( size_type N ) const
{

    if ( this->vm()["crb.check.rb"].template as<int>() == 0 && this->vm()["crb.check.residual"].template as<int>()==0 )
        return;

    std::cout << "  -- check reduced basis\n";


    LOG(INFO) << "----------------------------------------------------------------------\n";

    // check that for each mu associated to a basis function of \f$W_N\f$
    //for( int k = std::max(0,(int)N-2); k < N; ++k )
    for ( size_type k = 0; k < N; ++k )
    {
        LOG(INFO) << "**********************************************************************\n";
        parameter_type const& mu = M_WNmu->at( k );
        std::vector< vectorN_type > uN; //uN.resize( N );
        std::vector< vectorN_type > uNdu; //( N );
        std::vector< vectorN_type > uNold;
        std::vector< vectorN_type > uNduold;
        auto tuple = lb( N, mu, uN, uNdu, uNold, uNduold );
        double s = tuple.template get<0>();
        auto error_estimation = delta( N, mu, uN, uNdu , uNold, uNduold );
        double err = error_estimation.template get<0>() ;


#if 0
        //if (  err > 1e-5 )
        // {
        std::cout << "[check] error bounds are not < 1e-10\n";
        std::cout << "[check] k = " << k << "\n";
        std::cout << "[check] mu = " << mu << "\n";
        std::cout << "[check] delta = " <<  err << "\n";
        std::cout << "[check] uN( " << k << " ) = " << uN( k ) << "\n";
#endif
        // }
        double sfem = M_model->output( M_output_index, mu );

        int size = mu.size();
        std::cout<<"    o mu = [ ";

        for ( int i=0; i<size-1; i++ ) std::cout<< mu[i] <<" , ";

        std::cout<< mu[size-1]<<" ]"<<std::endl;

        LOG(INFO) << "[check] s= " << s << " +- " << err  << " | sfem= " << sfem << " | abs(sfem-srb) =" << math::abs( sfem - s ) << "\n";
        std::cout <<"[check] s = " << s << " +- " << err  << " | sfem= " << sfem << " | abs(sfem-srb) =" << math::abs( sfem - s )<< "\n";



        if ( this->vm()["crb.check.residual"].template as<int>() == 1 )
        {
	     std::vector < std::vector<double> > primal_residual_coefficients = error_estimation.template get<1>();
	     std::vector < std::vector<double> > dual_residual_coefficients = error_estimation.template get<2>();
	     //std::vector<double> coefficients = error_estimation.template get<1>();
            //checkResidual( mu , coefficients );

        }


    }

    LOG(INFO) << "----------------------------------------------------------------------\n";

}

template<typename TruthModelType>
void
CRB_heatns<TruthModelType>::computeErrorEstimationEfficiencyIndicator ( parameterspace_ptrtype const& Dmu, double& max_ei, double& min_ei,int N )
{
    std::vector< vectorN_type > uN; //( N );
    std::vector< vectorN_type > uNdu;//( N );

    //sampling of parameter space Dmu
    sampling_ptrtype Sampling;
    Sampling = sampling_ptrtype( new sampling_type( M_Dmu ) );
    //Sampling->equidistribute( N );
    Sampling->randomize( N );
    //Sampling->logEquidistribute( N );

    int RBsize = M_WNmu->size();

    y_type ei( Sampling->size() );

    for ( size_type k = 0; k < Sampling->size(); ++k )
    {
        parameter_type const& mu = M_Xi->at( k );
        double s = lb( RBsize, mu, uN, uNdu );//output
        double sfem = M_model->output( M_output_index, mu ); //true ouput
        double error_estimation = delta( RBsize,mu,uN,uNdu );
        ei( k ) = error_estimation/math::abs( sfem-s );
        std::cout<<" efficiency indicator = "<<ei( k )<<" for parameters {";

        for ( int i=0; i<mu.size(); i++ ) std::cout<<mu[i]<<" ";

        std::cout<<"}  --  |sfem - s| = "<<math::abs( sfem-s )<<" and error estimation = "<<error_estimation<<std::endl;
    }

    Eigen::MatrixXf::Index index_max_ei;
    max_ei = ei.array().abs().maxCoeff( &index_max_ei );
    Eigen::MatrixXf::Index index_min_ei;
    min_ei = ei.array().abs().minCoeff( &index_min_ei );

    std::cout<<"[EI] max_ei = "<<max_ei<<" and min_ei = "<<min_ei<<min_ei<<" sampling size = "<<N<<std::endl;

}//end of computeErrorEstimationEfficiencyIndicator





template< typename TruthModelType>
double
CRB_heatns<TruthModelType>::correctionTerms(parameter_type const& mu, std::vector< vectorN_type > const & uN, std::vector< vectorN_type > const & uNdu,  std::vector<vectorN_type> const & uNold, int const k ) const
{


    int N = uN[0].size();

    matrixN_type Aprdu ( (int)N, (int)N ) ;
    matrixN_type Mprdu ( (int)N, (int)N ) ;
    vectorN_type Fdu ( (int)N );
    vectorN_type du ( (int)N );
    vectorN_type pr ( (int)N );
    vectorN_type oldpr ( (int)N );


    double time = 1e30;

    theta_vector_type theta_aq;
    theta_vector_type theta_mq;
    std::vector<theta_vector_type> theta_fq;


    double correction=0;

    if( M_model->isSteady() )
    {

        Aprdu.setZero( N , N );
	Fdu.setZero( N );

        boost::tie( theta_mq, theta_aq, theta_fq ) = M_model->computeThetaq( mu ,time);

	for(size_type q = 0;q < M_model->Ql(0); ++q)
	    Fdu += theta_fq[0][q]*M_Fq_du[q].head(N);

	for(size_type q = 0;q < M_model->Qa(); ++q)
	    Aprdu += theta_aq[q]*M_Aq_pr_du[q].block(0,0,N,N);

	du = uNdu[0];
	pr = uN[0];
	correction = -( Fdu.dot( du ) - du.dot( Aprdu*pr )  );

    }
    else
    {
        std::cout << "ERROR : I'm not transient !!!" << std::endl;
    }

    return correction;

}

template<typename TruthModelType>
boost::tuple<double,double>
CRB_heatns<TruthModelType>::lb( size_type N, parameter_type const& mu, std::vector< vectorN_type > & uN, std::vector< vectorN_type > & uNdu,  std::vector<vectorN_type> & uNold, std::vector<vectorN_type> & uNduold,int K  ) const
{

    bool save_output_behavior = this->vm()["crb.save-output-behavior"].template as<bool>();

    //if K>0 then the time at which we want to evaluate output is defined by
    //time_for_output = K * time_step
    //else it's the default value and in this case we take final time
    double time_for_output;

    double time_step;
    int model_K;
    size_type Qm;

    if ( M_model->isSteady() )
    {
        time_step = 1e30;
        time_for_output = 1e30;
        Qm = 0;
        model_K=1; //only one time step
    }

    else
    {
        std::cout << "ERROR : I'm not transient !!!" << std::endl;
    }


    if ( N > M_N ) N = M_N;

    uN.resize( model_K );
    uNdu.resize( model_K );
    uNold.resize( model_K );
    uNduold.resize( model_K );


    int index=0;
    BOOST_FOREACH( auto elem, uN )
    {
        uN[index].resize( N );
        uNdu[index].resize( N );
        uNold[index].resize( N );
        uNduold[index].resize( N );
        index++;
    }

    theta_vector_type theta_aq;
    theta_vector_type theta_mq;
    std::vector<theta_vector_type> theta_fq, theta_lq;

    matrixN_type A ( ( int )N, ( int )N ) ;
    vectorN_type F ( ( int )N );
    vectorN_type L ( ( int )N );

    matrixN_type Adu ( ( int )N, ( int )N ) ;
    matrixN_type Mdu ( ( int )N, ( int )N ) ;
    vectorN_type Fdu ( ( int )N );
    vectorN_type Ldu ( ( int )N );

    matrixN_type Aprdu( ( int )N, ( int )N );
    matrixN_type Mprdu( ( int )N, ( int )N );

    if ( !M_model->isSteady() )
    {
        std::cout << "ERROR : I'm not transient !!!" << std::endl;
    }

    //-- end of initialization step

    //vector containing outputs from time=time_step until time=time_for_output
    std::vector<double>output_time_vector;
    double output;
    int time_index=0;

    for ( double time=time_step; time<=time_for_output; time+=time_step )
    {

        boost::tie( theta_mq, theta_aq, theta_fq ) = M_model->computeThetaq( mu ,time );

        A.setZero( N,N );
        for ( size_type q = 0; q < M_model->Qa(); ++q )
        {
            A += theta_aq[q]*M_Aq_pr[q].block( 0,0,N,N );
        }

        F.setZero( N );

        for ( size_type q = 0; q < M_model->Ql( 0 ); ++q )
        {
            F += theta_fq[0][q]*M_Fq_pr[q].head( N );
        }

        for ( size_type q = 0; q < Qm; ++q )
        {
            A += theta_mq[q]*M_Mq_pr[q].block( 0,0,N,N )/time_step;
            F += theta_mq[q]*M_Mq_pr[q].block( 0,0,N,N )*uNold[time_index]/time_step;
        }

        uN[time_index] = A.lu().solve( F );

        if ( time_index<model_K-1 )
        {
            uNold[time_index+1] = uN[time_index];
        }

        L.setZero( N );

        for ( size_type q = 0; q < M_model->Ql( M_output_index ); ++q )
        {
            L += theta_fq[M_output_index][q]*M_Lq_pr[q].head( N );
        }

        output = L.dot( uN[time_index] );

        output_time_vector.push_back( output );

        time_index++;
    }

    time_index--;

#if 0
    //compute conditioning of matrix A
    Eigen::SelfAdjointEigenSolver< matrixN_type > eigen_solver;
    eigen_solver.compute( A );
    int number_of_eigenvalues =  eigen_solver.eigenvalues().size();
    //we copy eigenvalues in a std::vector beacause it's easier to manipulate it
    std::vector<double> eigen_values( number_of_eigenvalues );

    for ( int i=0; i<number_of_eigenvalues; i++ )
    {
        if ( imag( eigen_solver.eigenvalues()[i] )>1e-12 )
        {
            throw std::logic_error( "[CRB::lb] ERROR : complex eigenvalues were found" );
        }

        eigen_values[i]=real( eigen_solver.eigenvalues()[i] );
    }

    int position_of_largest_eigenvalue=number_of_eigenvalues-1;
    int position_of_smallest_eigenvalue=0;
    double eig_max = eigen_values[position_of_largest_eigenvalue];
    double eig_min = eigen_values[position_of_smallest_eigenvalue];
    double condition_number = eig_max / eig_min;
    //end of computation of conditionning
#endif
    double condition_number = 0;
    double s_wo_correction = L.dot( uN [time_index] );
    double s = s_wo_correction ;


    //now the dual problem
    if (  this->vm()["crb.solve-dual-problem"].template as<bool>() || M_error_type == CRB_RESIDUAL || M_error_type == CRB_RESIDUAL_SCM || !M_compute_variance )
    {
        double time;

        if ( M_model->isSteady() )
        {
            time = 1e30;

            boost::tie( theta_mq, theta_aq, theta_fq ) = M_model->computeThetaq( mu ,time );
            Adu.setZero( N,N );
            Ldu.setZero( N );

            for ( size_type q = 0; q < M_model->Qa(); ++q )
            {
                Adu += theta_aq[q]*M_Aq_du[q].block( 0,0,N,N );
            }

            for ( size_type q = 0; q < M_model->Ql( M_output_index ); ++q )
            {
                Ldu += theta_fq[M_output_index][q]*M_Lq_du[q].head( N );
            }

            uNdu[0] = Adu.lu().solve( -Ldu );
        }


        else
        {
            std::cout << "ERROR : I'm not transient !!!" << std::endl;
        }//end of non steady case


        time_index=0;

        for ( double time=time_step; time<=time_for_output; time+=time_step )
        {
            int k = time_index+1;
            output_time_vector[time_index]+=correctionTerms(mu, uN , uNdu, uNold, k );
            time_index++;
        }

    }//end of if ( solve_dual_problem || M_error_type == CRB_RESIDUAL || M_error_type == CRB_RESIDUAL_SCM )

    if( M_compute_variance )
    {
        time_index=0;
        for ( double time=time_step; time<=time_for_output; time+=time_step )
        {

            vectorN_type uNsquare = uN[time_index].array().pow(2);
            double first = uNsquare.dot( M_variance_matrix_phi.diagonal() );

            double second = 0;
            for(int k = 1; k <= N-1; ++k)
            {
                for(int j = 1; j <= N-k; ++j)
                    second += 2 * uN[time_index](k-1) * uN[time_index](k+j-1) * M_variance_matrix_phi(k-1 , j+k-1) ;
            }
            output_time_vector[time_index] = first + second;
            time_index++;
        }
    }


    if ( save_output_behavior )
    {
        time_index=0;
        std::ofstream file_output;
        std::string mu_str;

        for ( int i=0; i<mu.size(); i++ )
        {
            mu_str= mu_str + ( boost::format( "_%1%" ) %mu[i] ).str() ;
        }

        std::string name = "output_evolution" + mu_str;
        file_output.open( name.c_str(),std::ios::out );

        for ( double time=time_step; time<=time_for_output; time+=time_step )
        {
            file_output<<time<<"\t"<<output_time_vector[time_index]<<"\n";
            time_index++;
        }

        file_output.close();
    }

    int size=output_time_vector.size();
    return boost::make_tuple( output_time_vector[size-1], condition_number);


}



template<typename TruthModelType>
typename CRB_heatns<TruthModelType>::error_estimation_type
CRB_heatns<TruthModelType>::delta( size_type N,
                            parameter_type const& mu,
                            std::vector<vectorN_type> const& uN,
                            std::vector<vectorN_type> const& uNdu,
                            std::vector<vectorN_type> const& uNold,
                            std::vector<vectorN_type> const& uNduold,
                            int k ) const
{
#if 0

    std::vector< std::vector<double> > primal_residual_coeffs;
    std::vector< std::vector<double> > dual_residual_coeffs;

    double delta_pr=0;
    double delta_du=0;
    if ( M_error_type == CRB_NO_RESIDUAL )
        return boost::make_tuple( -1,primal_residual_coeffs,dual_residual_coeffs,delta_pr,delta_du );

    else if ( M_error_type == CRB_EMPIRICAL )
        return boost::make_tuple( empiricalErrorEstimation ( N, mu , k ) , primal_residual_coeffs, dual_residual_coeffs , delta_pr, delta_du);

    else
    {
        //we assume that we want estimate the error committed on the final output

        double primal_sum=0;
        double dual_sum=0;


        //vectors to store residual coefficients
        primal_residual_coeffs.resize( 1 );
        dual_residual_coeffs.resize( 1 );

        if ( M_model->isSteady() )
        {
            residual_error_type pr = steadyPrimalResidual( N, mu, uN[0], 0. );
        }
        else
        {
            std::cout << "ERROR : I'm not transient !!!" << std::endl;
        }

        double dual_residual=0;

        if ( M_model->isSteady() )
        {
            residual_error_type du = steadyDualResidual( N, mu, uNdu[0], uNduold[0]);
            dual_sum += du.template get<0>();
            dual_residual_coeffs[0].resize( du.template get<1>().size() );
            dual_residual_coeffs[0] = du.template get<1>();
        }
        else
        {
            std::cout << "ERROR : I'm not transient !!!" << std::endl;
        }


        double alphaA=1,alphaM=1;

        if ( M_error_type == CRB_RESIDUAL_SCM )
        {
            double alphaA_up, lbti;
            M_scmA->setScmForMassMatrix( false );
            boost::tie( alphaA, lbti ) = M_scmA->lb( mu );
            boost::tie( alphaA_up, lbti ) = M_scmA->ub( mu );
            std::cout << "alphaA_lo = " << alphaA << " alphaA_hi = " << alphaA_up << "\n";

            if ( ! M_model->isSteady() )
            {
                M_scmM->setScmForMassMatrix( true );
                double alphaM_up, lbti;
                boost::tie( alphaM, lbti ) = M_scmM->lb( mu );
                boost::tie( alphaM_up, lbti ) = M_scmM->ub( mu );
                std::cout << "alphaM_lo = " << alphaM << " alphaM_hi = " << alphaM_up << "\n";
            }

        }

        double upper_bound;

        if ( M_model->isSteady() )
        {
            delta_pr = math::sqrt( primal_sum ) / math::sqrt( alphaA );
            delta_du = math::sqrt( dual_sum ) / math::sqrt( alphaA );
            upper_bound = delta_pr * delta_du;
        }

        else
        {
            std::cout << "ERROR : I'm not transient !!!" << std::endl;
        }

        return boost::make_tuple( upper_bound, primal_residual_coeffs, dual_residual_coeffs , delta_pr, delta_du );

    }//end of else

#else

    double upper_bound;
    std::vector< std::vector<double> > primal_residual_coeffs;
    std::vector< std::vector<double> > dual_residual_coeffs;
    double delta_pr=0;
    double delta_du=0;

    return boost::make_tuple( upper_bound, primal_residual_coeffs, dual_residual_coeffs , delta_pr, delta_du );

#endif
}

template<typename TruthModelType>
typename CRB_heatns<TruthModelType>::max_error_type
CRB_heatns<TruthModelType>::maxErrorBounds( size_type N ) const
{
    bool seek_mu_in_complement = this->vm()["crb.seek-mu-in-complement"].template as<bool>() ;

    std::vector< vectorN_type > uN;
    std::vector< vectorN_type > uNdu;
    std::vector< vectorN_type > uNold;
    std::vector< vectorN_type > uNduold;

    y_type err( M_Xi->size() );
    y_type vect_delta_pr( M_Xi->size() );
    y_type vect_delta_du( M_Xi->size() );
    std::vector<double> check_err( M_Xi->size() );

    double delta_pr=0;
    double delta_du=0;
    if ( M_error_type == CRB_EMPIRICAL )
    {
        if ( M_WNmu->size()==1 )
        {
            parameter_type mu ( M_Dmu );
            size_type id;
            boost::tie ( mu, id ) = M_Xi->max();
            return boost::make_tuple( 1e5, M_Xi->at( id ), id , delta_pr, delta_du);
        }

        else
        {
            err.resize( M_WNmu_complement->size() );
            check_err.resize( M_WNmu_complement->size() );

            for ( size_type k = 0; k < M_WNmu_complement->size(); ++k )
            {
                parameter_type const& mu = M_WNmu_complement->at( k );
                //double _err = delta( N, mu, uN, uNdu, uNold, uNduold, k);
                auto error_estimation = delta( N, mu, uN, uNdu, uNold, uNduold, k );
                double _err = error_estimation.template get<0>();
                vect_delta_pr( k ) = error_estimation.template get<3>();
                vect_delta_du( k ) = error_estimation.template get<4>();
                err( k ) = _err;
                check_err[k] = _err;
            }
        }

    }//end of if ( M_error_type == CRB_EMPIRICAL)

    else
    {
        if ( seek_mu_in_complement )
        {
            err.resize( M_WNmu_complement->size() );
            check_err.resize( M_WNmu_complement->size() );

            for ( size_type k = 0; k < M_WNmu_complement->size(); ++k )
            {
                parameter_type const& mu = M_WNmu_complement->at( k );
                lb( N, mu, uN, uNdu , uNold ,uNduold );
                auto error_estimation = delta( N, mu, uN, uNdu, uNold, uNduold, k );
                double _err = error_estimation.template get<0>();
                vect_delta_pr( k ) = error_estimation.template get<3>();
                vect_delta_du( k ) = error_estimation.template get<4>();
                err( k ) = _err;
                check_err[k] = _err;
            }
        }//end of seeking mu in the complement

        else
        {
            for ( size_type k = 0; k < M_Xi->size(); ++k )
            {
                //std::cout << "--------------------------------------------------\n";
                parameter_type const& mu = M_Xi->at( k );
                //std::cout << "[maxErrorBounds] mu=" << mu << "\n";
                lb( N, mu, uN, uNdu , uNold ,uNduold );
                //auto tuple = lb( N, mu, uN, uNdu , uNold ,uNduold );
                //double o = tuple.template get<0>();
                //std::cout << "[maxErrorBounds] output=" << o << "\n";
                auto error_estimation = delta( N, mu, uN, uNdu, uNold, uNduold, k );
                double _err = error_estimation.template get<0>();
                vect_delta_pr( k ) = error_estimation.template get<3>();
                vect_delta_du( k ) = error_estimation.template get<4>();
                //std::cout << "[maxErrorBounds] error=" << _err << "\n";
                err( k ) = _err;
                check_err[k] = _err;
            }
        }//else ( seek_mu_in_complement )
    }//else

    Eigen::MatrixXf::Index index;
    double maxerr = err.array().abs().maxCoeff( &index );
    delta_pr = vect_delta_pr( index );
    delta_du = vect_delta_du( index );
    parameter_type mu;

    std::vector<double>::iterator it = std::max_element( check_err.begin(), check_err.end() );
    int check_index = it - check_err.begin() ;
    double check_maxerr = *it;

    if ( index != check_index || maxerr != check_maxerr )
    {
        std::cout<<"[CRB::maxErrorBounds] index = "<<index<<" / check_index = "<<check_index<<"   and   maxerr = "<<maxerr<<" / "<<check_maxerr<<std::endl;
        throw std::logic_error( "[CRB::maxErrorBounds] index and check_index have different values" );
    }

    int _index=0;

    if ( seek_mu_in_complement )
    {
        LOG(INFO) << "[maxErrorBounds] WNmu_complement N=" << N << " max Error = " << maxerr << " at index = " << index << "\n";
        mu = M_WNmu_complement->at( index );
        _index = M_WNmu_complement->indexInSuperSampling( index );
    }

    else
    {
        LOG(INFO) << "[maxErrorBounds] N=" << N << " max Error = " << maxerr << " at index = " << index << "\n";
        mu = M_Xi->at( index );
        _index = index;
    }

    return boost::make_tuple( maxerr, mu , _index , delta_pr, delta_du);


}
template<typename TruthModelType>
void
CRB_heatns<TruthModelType>::orthonormalize( size_type N, wn_type& wn, int Nm )
{
    std::cout << "  -- orthonormalization (Gram-Schmidt)\n";
    Debug ( 12000 ) << "[CRB::orthonormalize] orthonormalize basis for N=" << N << "\n";
    Debug ( 12000 ) << "[CRB::orthonormalize] orthonormalize basis for WN="
                    << wn.size() << "\n";
    Debug ( 12000 ) << "[CRB::orthonormalize] starting ...\n";


    for ( size_type i = 0; i < N; ++i )
    {
        for ( size_type j = std::max( i+1,N-Nm ); j < N; ++j )
        {
            value_type __rij_pr = M_model->scalarProduct(  wn[i], wn[ j ] );
            wn[j].add( -__rij_pr, wn[i] );
        }
    }

    // normalize
    for ( size_type i =N-Nm; i < N; ++i )
    {
        value_type __rii_pr = math::sqrt( M_model->scalarProduct(  wn[i], wn[i] ) );
        wn[i].scale( 1./__rii_pr );
    }

    Debug ( 12000 ) << "[CRB::orthonormalize] finished ...\n";
    Debug ( 12000 ) << "[CRB::orthonormalize] copying back results in basis\n";

    if ( this->vm()["crb.check.gs"].template as<int>() )
        checkOrthonormality( N , wn );

}

template <typename TruthModelType>
void
CRB_heatns<TruthModelType>::checkOrthonormality ( int N, const wn_type& wn ) const
{

    if ( wn.size()==0 )
    {
        throw std::logic_error( "[CRB::checkOrthonormality] ERROR : size of wn is zero" );
    }

    if ( orthonormalize_primal*orthonormalize_dual==0 )
    {
        std::cout<<"Warning : calling checkOrthonormality is called but ";
        std::cout<<" orthonormalize_dual = "<<orthonormalize_dual;
        std::cout<<" and orthonormalize_primal = "<<orthonormalize_primal<<std::endl;
    }

    matrixN_type A, I;
    A.setZero( N, N );
    I.setIdentity( N, N );

    for ( int i = 0; i < N; ++i )
    {
        for ( int j = 0; j < N; ++j )
        {
            A( i, j ) = M_model->scalarProduct(  wn[i], wn[j] );
        }
    }

    A -= I;
    Debug( 12000 ) << "orthonormalization: " << A.norm() << "\n";
    std::cout << "    o check : " << A.norm() << " (should be 0)\n";
    //FEELPP_ASSERT( A.norm() < 1e-14 )( A.norm() ).error( "orthonormalization failed.");
}


template <typename TruthModelType>
void
CRB_heatns<TruthModelType>::exportBasisFunctions( const export_vector_wn_type& export_vector_wn )const
{


}


//error estimation only to build reduced basis
template <typename TruthModelType>
typename CRB_heatns<TruthModelType>::value_type
CRB_heatns<TruthModelType>::empiricalErrorEstimation ( int Nwn, parameter_type const& mu , int second_index ) const
{
    std::vector<vectorN_type> Un;
    std::vector<vectorN_type> Undu;
    std::vector<vectorN_type> Unold;
    std::vector<vectorN_type> Unduold;
    auto o = lb( Nwn, mu, Un, Undu, Unold, Unduold  );
    double sn = o.template get<0>();

    int nb_element =Nwn/M_factor*( M_factor>0 ) + ( Nwn+M_factor )*( M_factor<0 && ( int )Nwn>( -M_factor ) ) + 1*( M_factor<0 && ( int )Nwn<=( -M_factor ) )  ;

    std::vector<vectorN_type> Un2;
    std::vector<vectorN_type> Undu2;//( nb_element );

    //double output_smaller_basis = lb(nb_element, mu, Un2, Undu2, Unold, Unduold);
    auto tuple = lb( nb_element, mu, Un2, Undu2, Unold, Unduold );
    double output_smaller_basis = tuple.template get<0>();

    double error_estimation = math::abs( sn-output_smaller_basis );

    return error_estimation;

}


template<typename TruthModelType>
typename CRB_heatns<TruthModelType>::residual_error_type
CRB_heatns<TruthModelType>::steadyPrimalResidual( int Ncur,parameter_type const& mu, vectorN_type const& Un, double time ) const
{

    theta_vector_type theta_aq;
    std::vector<theta_vector_type> theta_fq, theta_lq;
    boost::tie( boost::tuples::ignore, theta_aq, theta_fq ) = M_model->computeThetaq( mu , time );


    int __QLhs = M_model->Qa();
    int __QRhs = M_model->Ql( 0 );
    int __N = Ncur;

    // primal terms
    value_type __c0_pr = 0.0;

    for ( int __q1 = 0; __q1 < __QRhs; ++__q1 )
    {
        value_type fq1 = theta_fq[0][__q1];

        for ( int __q2 = 0; __q2 < __QRhs; ++__q2 )
        {
            value_type fq2 = theta_fq[0][__q2];
            __c0_pr += M_C0_pr[__q1][__q2]*fq1*fq2;
        }
    }

    value_type __lambda_pr = 0.0;
    value_type __gamma_pr = 0.0;

    for ( int __q1 = 0; __q1 < __QLhs; ++__q1 )
    {
        value_type a_q1 = theta_aq[__q1];

        for ( int __q2 = 0; __q2 < __QRhs; ++__q2 )
        {
            value_type f_q2 = theta_fq[0][__q2];
            __lambda_pr += a_q1*f_q2*M_Lambda_pr[__q1][__q2].head( __N ).dot( Un );
        }

        for ( int __q2 = 0; __q2 < __QLhs; ++__q2 )
        {
            value_type a_q2 = theta_aq[__q2];
            auto m = M_Gamma_pr[__q1][__q2].block( 0,0,__N,__N )*Un;
            __gamma_pr += a_q1 * a_q2 * Un.dot( m );
        }
    }

    //value_type delta_pr = math::sqrt( math::abs(__c0_pr+__lambda_pr+__gamma_pr) );
    value_type delta_pr = math::abs( __c0_pr+__lambda_pr+__gamma_pr ) ;


#if(0)

    if ( !boost::math::isfinite( delta_pr ) )
    {
        std::cout << "delta_pr is not finite:\n"
                  << "  - c0_pr = " << __c0_pr << "\n"
                  << "  - lambda_pr = " << __lambda_pr << "\n"
                  << "  - gamma_pr = " << __gamma_pr << "\n";
        std::cout << " - theta_aq = " << theta_aq << "\n"
                  << " - theta_fq = " << theta_fq << "\n";
        std::cout << " - Un : " << Un << "\n";
    }

    //std::cout << "delta_pr=" << delta_pr << std::endl;
#endif

    std::vector<double> coeffs_vector;
    coeffs_vector.push_back( __c0_pr );
    coeffs_vector.push_back( __lambda_pr );
    coeffs_vector.push_back( __gamma_pr );

    return boost::make_tuple( delta_pr,coeffs_vector );
}





template<typename TruthModelType>
typename CRB_heatns<TruthModelType>::residual_error_type
CRB_heatns<TruthModelType>::steadyDualResidual( int Ncur,parameter_type const& mu, vectorN_type const& Undu, double time ) const
{


    int __QLhs = M_model->Qa();
    int __QOutput = M_model->Ql( M_output_index );
    int __N = Ncur;

    theta_vector_type theta_aq;
    theta_vector_type theta_mq;
    std::vector<theta_vector_type> theta_fq, theta_lq;
    boost::tie( boost::tuples::ignore, theta_aq, theta_fq ) = M_model->computeThetaq( mu , time );

    value_type __c0_du = 0.0;

    for ( int __q1 = 0; __q1 < __QOutput; ++__q1 )
    {
        value_type fq1 = theta_fq[M_output_index][__q1];

        for ( int __q2 = 0; __q2 < __QOutput; ++__q2 )
        {
            value_type fq2 = theta_fq[M_output_index][__q2];
            __c0_du += M_C0_du[__q1][__q2]*fq1*fq2;
        }
    }

    value_type __lambda_du = 0.0;
    value_type __gamma_du = 0.0;

    for ( int __q1 = 0; __q1 < __QLhs; ++__q1 )
    {
        value_type a_q1 = theta_aq[__q1];

        for ( int __q2 = 0; __q2 < __QOutput; ++__q2 )
        {
            value_type a_q2 = theta_fq[M_output_index][__q2]*a_q1;
            __lambda_du += a_q2 * M_Lambda_du[__q1][__q2].head( __N ).dot( Undu );
        }

        for ( int __q2 = 0; __q2 < __QLhs; ++__q2 )
        {
            value_type a_q2 = theta_aq[__q2]*a_q1;
            auto m = M_Gamma_du[ __q1][ __q2].block( 0,0,__N,__N )*Undu;
            __gamma_du += a_q2*Undu.dot( m );
        }
    }

    value_type delta_du = math::abs( __c0_du+__lambda_du+__gamma_du );
#if(0)

    if ( !boost::math::isfinite( delta_du ) )
    {
        std::cout << "delta_du is not finite:\n"
                  << "  - c0_du = " << __c0_du << "\n"
                  << "  - lambda_du = " << __lambda_du << "\n"
                  << "  - gamma_du = " << __gamma_du << "\n";
        std::cout << " - theta_aq = " << theta_aq << "\n"
                  << " - theta_fq = " << theta_fq << "\n";
        std::cout << " - Undu : " << Undu << "\n";
    }

#endif

    std::vector<double> coeffs_vector;
    coeffs_vector.push_back( __c0_du );
    coeffs_vector.push_back( __lambda_du );
    coeffs_vector.push_back( __gamma_du );

    return boost::make_tuple( delta_du,coeffs_vector );
}

template<typename TruthModelType>
void
CRB_heatns<TruthModelType>::offlineResidual( int Ncur ,int number_of_added_elements )
{
    std::cout << "------->CRB_heatns<TruthModelType>::offlineResidual( int Ncur ,int number_of_added_elements )\n";
    return offlineResidual( Ncur, mpl::bool_<model_type::is_time_dependent>(), number_of_added_elements );
}

template<typename TruthModelType>
void
CRB_heatns<TruthModelType>::offlineResidual( int Ncur, mpl::bool_<true>, int number_of_added_elements )
{
    std::cout << "------->CRB_heatns<TruthModelType>::offlineResidual( int Ncur, mpl::bool_<true>, int number_of_added_elements )\n";

    boost::timer ti;
    int __QLhs = M_model->Qa();
    int __QRhs = M_model->Ql( 0 );
    int __QOutput = M_model->Ql( M_output_index );
    int __Qm = M_model->Qm();
    int __N = Ncur;
    std::cout << "     o N=" << Ncur << " QLhs=" << __QLhs
              << " QRhs=" << __QRhs << " Qoutput=" << __QOutput
              << " Qm=" << __Qm << "\n";
    vector_ptrtype __X( M_backend->newVector( M_model->functionSpace() ) );
    vector_ptrtype __Fdu( M_backend->newVector( M_model->functionSpace() ) );
    vector_ptrtype __Z1(  M_backend->newVector( M_model->functionSpace() ) );
    vector_ptrtype __Z2(  M_backend->newVector( M_model->functionSpace() ) );
    vector_ptrtype __W(  M_backend->newVector( M_model->functionSpace() ) );
    namespace ublas = boost::numeric::ublas;

    std::vector<sparse_matrix_ptrtype> Aq;
    std::vector<sparse_matrix_ptrtype> Mq;
    std::vector<std::vector<vector_ptrtype> > Fq;
    boost::tie( Mq, Aq, Fq ) = M_model->computeAffineDecomposition();
    __X->zero();
    __X->add( 1.0 );
    //std::cout << "measure of domain= " << M_model->scalarProduct( __X, __X ) << "\n";
#if 0
    ublas::vector<value_type> mu( P );

    for ( int q = 0; q < P; ++q )
    {
        mu[q] = mut( 0.0 );
    }

#endif

    std::cout << "------->offlineResidual( Ncur, mpl::bool_<false>(), number_of_added_elements );\n";
    offlineResidual( Ncur, mpl::bool_<false>(), number_of_added_elements );

    for ( int __q1 = 0; __q1 < __Qm; ++__q1 )
    {
        for ( int __q2 = 0; __q2 < __QRhs; ++__q2 )
        {
            M_Cmf_pr[__q1][__q2].conservativeResize( __N );

            for ( int elem=__N-number_of_added_elements; elem<__N; elem++ )
            {
                *__X=M_WN[elem];
                Mq[__q1]->multVector(  __X, __W );
                __W->scale( -1. );
                M_model->l2solve( __Z1, __W );

                M_model->l2solve( __Z2, Fq[0][__q2] );

                M_Cmf_pr[ __q1][ __q2]( elem ) = 2.0*M_model->scalarProduct( __Z1, __Z2 );

            }
        }
    }

    std::cout << "     o M_Cmf_pr updated in " << ti.elapsed() << "s\n";
    ti.restart();


    for ( int __q1 = 0; __q1 < __Qm; ++__q1 )
    {
        for ( int __q2 = 0; __q2 < __QLhs; ++__q2 )
        {
            M_Cma_pr[__q1][__q2].conservativeResize( __N, __N );

            for ( int elem=__N-number_of_added_elements; elem<__N; elem++ )
            {
                *__X=M_WN[elem];
                Mq[__q1]->multVector(  __X, __W );
                __W->scale( -1. );
                M_model->l2solve( __Z1, __W );

                for ( int __l = 0; __l < ( int )__N; ++__l )
                {
                    *__X=M_WN[__l];
                    Aq[__q2]->multVector(  __X, __W );
                    __W->scale( -1. );
                    M_model->l2solve( __Z2, __W );

                    M_Cma_pr[ __q1][ __q2]( elem,__l ) = 2.0*M_model->scalarProduct( __Z1, __Z2 );
                }
            }//end of loop over elem

            for ( int __j = 0; __j < ( int )__N; ++__j )
            {
                *__X=M_WN[__j];
                Mq[__q1]->multVector(  __X, __W );
                __W->scale( -1. );
                M_model->l2solve( __Z1, __W );

                for ( int elem=__N-number_of_added_elements; elem<__N; elem++ )
                {
                    *__X=M_WN[elem];
                    Aq[__q2]->multVector(  __X, __W );
                    __W->scale( -1. );
                    M_model->l2solve( __Z2, __W );

                    M_Cma_pr[ __q1][ __q2]( __j,elem ) = 2.0*M_model->scalarProduct( __Z1, __Z2 );
                }
            }

        }// on N1
    } // on q1

    std::cout << "     o M_Cma_pr updated in " << ti.elapsed() << "s\n";
    ti.restart();

    for ( int __q1 = 0; __q1 < __Qm; ++__q1 )
    {
        for ( int __q2 = 0; __q2 < __Qm; ++__q2 )
        {
            M_Cmm_pr[__q1][__q2].conservativeResize( __N, __N );

            for ( int elem=__N-number_of_added_elements; elem<__N; elem++ )
            {
                *__X=M_WN[elem];
                Mq[__q1]->multVector(  __X, __W );
                __W->scale( -1. );
                M_model->l2solve( __Z1, __W );

                for ( int __l = 0; __l < ( int )__N; ++__l )
                {
                    *__X=M_WN[__l];
                    Mq[__q2]->multVector(  __X, __W );
                    __W->scale( -1. );
                    M_model->l2solve( __Z2, __W );

                    M_Cmm_pr[ __q1][ __q2]( elem,__l ) = M_model->scalarProduct( __Z1, __Z2 );
                }
            }//end of loop over elem

            for ( int __j = 0; __j < ( int )__N; ++__j )
            {
                *__X=M_WN[__j];
                Mq[__q1]->multVector(  __X, __W );
                __W->scale( -1. );
                M_model->l2solve( __Z1, __W );

                for ( int elem=__N-number_of_added_elements; elem<__N; elem++ )
                {
                    *__X=M_WN[elem];
                    Mq[__q2]->multVector(  __X, __W );
                    __W->scale( -1. );
                    M_model->l2solve( __Z2, __W );

                    M_Cmm_pr[ __q1][ __q2]( __j,elem ) = M_model->scalarProduct( __Z1, __Z2 );

                }
            }

        }// on N1
    } // on q1

    std::cout << "     o M_Cmm_pr updated in " << ti.elapsed() << "s\n";
    ti.restart();


    sparse_matrix_ptrtype Atq1 = M_model->newMatrix();
    sparse_matrix_ptrtype Atq2 = M_model->newMatrix();
    sparse_matrix_ptrtype Mtq1 = M_model->newMatrix();
    sparse_matrix_ptrtype Mtq2 = M_model->newMatrix();
    //
    // Dual
    //


    LOG(INFO) << "[offlineResidual] Cmf_du Cma_du Cmm_du\n";

#if 0
    for ( int __q1 = 0; __q1 < __Qm; ++__q1 )
    {
        Mq[__q1]->transpose( Mtq1 );

        for ( int __q2 = 0; __q2 < __QOutput; ++__q2 )
        {
            M_Cmf_du[__q1][__q2].conservativeResize( __N );

            for ( int elem=__N-number_of_added_elements; elem<__N; elem++ )
            {
                *__X=M_WNdu[elem];
                Mtq1->multVector(  __X, __W );
                __W->scale( -1. );
                M_model->l2solve( __Z1, __W );

                *__Fdu = *Fq[M_output_index][__q2];
                __Fdu->scale( -1.0 );
                M_model->l2solve( __Z2, __Fdu );

                M_Cmf_du[ __q1][ __q2]( elem ) = 2.0*M_model->scalarProduct( __Z1, __Z2 );

            }
        } // q2
    } // q1
#endif

    for ( int __q1 = 0; __q1 < __Qm; ++__q1 )
    {
        Mq[__q1]->transpose( Mtq1 );

        for ( int __q2 = 0; __q2 < __QLhs; ++__q2 )
        {
            Aq[__q2]->transpose( Atq2 );
            M_Cma_du[__q1][__q2].conservativeResize( __N, __N );

            for ( int elem=__N-number_of_added_elements; elem<__N; elem++ )
            {

                *__X=M_WNdu[elem];
                Mtq1->multVector(  __X, __W );
                __W->scale( -1. );
                M_model->l2solve( __Z1, __W );

                for ( int __l = 0; __l < ( int )__N; ++__l )
                {
                    *__X=M_WNdu[__l];
                    Atq2->multVector(  __X, __W );
                    __W->scale( -1. );
                    M_model->l2solve( __Z2, __W );

                    M_Cma_du[ __q1][ __q2]( elem,__l ) = 2.0*M_model->scalarProduct( __Z1, __Z2 );
                }
            }

            for ( int __j = 0; __j < ( int )__N; ++__j )
            {
                *__X=M_WNdu[__j];
                Mtq1->multVector(  __X, __W );
                __W->scale( -1. );
                M_model->l2solve( __Z1, __W );

                for ( int elem=__N-number_of_added_elements; elem<__N; elem++ )
                {
                    *__X=M_WNdu[elem];
                    Atq2->multVector(  __X, __W );
                    __W->scale( -1. );
                    M_model->l2solve( __Z2, __W );

                    M_Cma_du[ __q1][ __q2]( __j,elem ) = 2.0*M_model->scalarProduct( __Z1, __Z2 );
                }
            }

        }// on N1
    } // on q1

    std::cout << "     o Cma_du updated in " << ti.elapsed() << "s\n";
    ti.restart();



    for ( int __q1 = 0; __q1 < __Qm; ++__q1 )
    {
        Mq[__q1]->transpose( Mtq1 );

        for ( int __q2 = 0; __q2 < __Qm; ++__q2 )
        {
            Mq[__q2]->transpose( Mtq2 );
            M_Cmm_du[__q1][__q2].conservativeResize( __N, __N );

            for ( int elem=__N-number_of_added_elements; elem<__N; elem++ )
            {

                *__X=M_WNdu[elem];
                Mtq1->multVector(  __X, __W );
                __W->scale( -1. );
                M_model->l2solve( __Z1, __W );

                for ( int __l = 0; __l < ( int )__N; ++__l )
                {
                    *__X=M_WNdu[__l];
                    Mtq2->multVector(  __X, __W );
                    __W->scale( -1. );
                    M_model->l2solve( __Z2, __W );

                    M_Cmm_du[ __q1][ __q2]( elem,__l ) = M_model->scalarProduct( __Z1, __Z2 );
                }
            }

            for ( int __j = 0; __j < ( int )__N; ++__j )
            {
                *__X=M_WNdu[__j];
                Mtq1->multVector(  __X, __W );
                __W->scale( -1. );
                M_model->l2solve( __Z1, __W );

                for ( int elem=__N-number_of_added_elements; elem<__N; elem++ )
                {
                    *__X=M_WNdu[elem];
                    Mtq2->multVector(  __X, __W );
                    __W->scale( -1. );
                    M_model->l2solve( __Z2, __W );

                    M_Cmm_du[ __q1][ __q2]( __j,elem ) = M_model->scalarProduct( __Z1, __Z2 );
                }
            }

        }// on N1
    } // on q1

    std::cout << "     o Cmm_du updated in " << ti.elapsed() << "s\n";
    ti.restart();

    LOG(INFO) << "[offlineResidual] Done.\n";

}


template<typename TruthModelType>
void
CRB_heatns<TruthModelType>::printErrorsDuringRbConstruction( void )
{

    if( M_rbconv_contains_primal_and_dual_contributions )
    {
        typedef convergence_type::left_map::const_iterator iterator;
        LOG(INFO)<<"\nMax error during offline stage\n";
	for(iterator it = M_rbconv.left.begin(); it != M_rbconv.left.end(); ++it)
            LOG(INFO)<<"N : "<<it->first<<"  -  maxerror : "<<it->second.template get<0>()<<"\n";

	LOG(INFO)<<"\nPrimal contribution\n";
	for(iterator it = M_rbconv.left.begin(); it != M_rbconv.left.end(); ++it)
            LOG(INFO)<<"N : "<<it->first<<"  -  delta_pr : "<<it->second.template get<1>()<<"\n";

	LOG(INFO)<<"\nDual contribution\n";
	for(iterator it = M_rbconv.left.begin(); it != M_rbconv.left.end(); ++it)
            LOG(INFO)<<"N : "<<it->first<<"  -  delta_du : "<<it->second.template get<2>()<<"\n";
	LOG(INFO)<<"\n";
    }
    else
        throw std::logic_error( "[CRB::printErrorsDuringRbConstruction] ERROR, the database is too old to print the error during offline step, use the option rebuild-database = true" );

}

template<typename TruthModelType>
void
CRB_heatns<TruthModelType>::offlineResidual( int Ncur, mpl::bool_<false> , int number_of_added_elements )
{

    std::cout <<"------->CRB_heatns<TruthModelType>::offlineResidual( int Ncur, mpl::bool_<false> , int number_of_added_elements )\n";

    boost::timer ti;
    int __QLhs = M_model->Qa();
    int __QRhs = M_model->Ql( 0 );
    int __QOutput = M_model->Ql( M_output_index );
    int __N = Ncur;
    std::cout << "     o N=" << Ncur << " QLhs=" << __QLhs
              << " QRhs=" << __QRhs << " Qoutput=" << __QOutput << "\n";
    vector_ptrtype __X( M_backend->newVector( M_model->functionSpace() ) );
    vector_ptrtype __Fdu( M_backend->newVector( M_model->functionSpace() ) );
    vector_ptrtype __Z1(  M_backend->newVector( M_model->functionSpace() ) );
    vector_ptrtype __Z2(  M_backend->newVector( M_model->functionSpace() ) );
    vector_ptrtype __W(  M_backend->newVector( M_model->functionSpace() ) );
    namespace ublas = boost::numeric::ublas;

    std::vector<sparse_matrix_ptrtype> Aq,Mq;
    std::vector<std::vector<vector_ptrtype> > Fq,Lq;
    boost::tie( Mq, Aq, Fq ) = M_model->computeAffineDecomposition();

    std::cout << "-------->boost::tie( Mq, Aq, Fq ) = M_model->computeAffineDecomposition(); DONE\n";

    __X->zero();
    __X->add( 1.0 );
    //std::cout << "measure of domain= " << M_model->scalarProduct( __X, __X ) << "\n";
#if 0
    ublas::vector<value_type> mu( P );

    for ( int q = 0; q < P; ++q )
    {
        mu[q] = mut( 0.0 );
    }

#endif

    // Primal
    // no need to recompute this term each time
    if ( Ncur == 1 )
    {
        LOG(INFO) << "[offlineResidual] Compute Primal residual data\n";
        LOG(INFO) << "[offlineResidual] C0_pr\n";
        std::cout << "-------->[offlineResidual] Compute Primal residual data\n";
        std::cout << "-------->[offlineResidual] C0_pr\n";

        // see above Z1 = C^-1 F and Z2 = F
        for ( int __q1 = 0; __q1 < __QRhs; ++__q1 )
        {
            std::cout << "-------->__q1 = " << __q1 << std::endl;
            //LOG(INFO) << "__Fq->norm1=" << Fq[0][__q1]->l2Norm() << "\n";
            M_model->l2solve( __Z1, Fq[0][__q1] );

            //for ( int __q2 = 0;__q2 < __q1;++__q2 )
            for ( int __q2 = 0; __q2 < __QRhs; ++__q2 )
            {
                std::cout << "-------->__q2 = " << __q2 << std::endl;
                //LOG(INFO) << "__Fq->norm 2=" << Fq[0][__q2]->l2Norm() << "\n";
                M_model->l2solve( __Z2, Fq[0][__q2] );
                //M_C0_pr[__q1][__q2] = M_model->scalarProduct( __X, Fq[0][__q2] );
                M_C0_pr[__q1][__q2] = M_model->scalarProduct( __Z1, __Z2 );
                //M_C0_pr[__q2][__q1] = M_C0_pr[__q1][__q2];
                //VLOG(1) << "M_C0_pr[" << __q1 << "][" << __q2 << "]=" << M_C0_pr[__q1][__q2] << "\n";
                //LOG(INFO) << "M_C0_pr[" << __q1 << "][" << __q2 << "]=" << M_C0_pr[__q1][__q2] << "\n";
            }

            //M_C0_pr[__q1][__q1] = M_model->scalarProduct( __X, __X );
        }
    }

    std::cout << "     o initialize offlineResidual in " << ti.elapsed() << "s\n";
    ti.restart();

#if 0
    parameter_type const& mu = M_WNmu->at( 0 );
    //std::cout << "[offlineResidual] mu=" << mu << "\n";
    theta_vector_type theta_aq;
    std::vector<theta_vector_type> theta_fq;
    boost::tie( boost::tuples::ignore, theta_aq, theta_fq ) = M_model->computeThetaq( mu );
    value_type __c0_pr = 0.0;

    for ( int __q1 = 0; __q1 < __QRhs; ++__q1 )
    {
        value_type fq1 = theta_fq[0][__q1];

        for ( int __q2 = 0; __q2 < __QRhs; ++__q2 )
        {
            value_type fq2 = theta_fq[0][__q2];
            __c0_pr += M_C0_pr[__q1][__q2]*fq1*fq2;
        }
    }

    //std::cout << "c0=" << __c0_pr << std::endl;

    sparse_matrix_ptrtype A;
    std::vector<vector_ptrtype> F;
    boost::tie( A, F ) = M_model->update( mu );
    M_model->l2solve( __X, F[0] );
    //std::cout << "c0 2 = " << M_model->scalarProduct( __X, __X ) << "\n";;
#endif

    //
    //  Primal
    //
    LOG(INFO) << "[offlineResidual] Lambda_pr, Gamma_pr\n";

    for ( int elem=__N-number_of_added_elements; elem<__N; elem++ )
    {
        *__X=M_WN[elem];

        for ( int __q1 = 0; __q1 < __QLhs; ++__q1 )
        {
            Aq[__q1]->multVector(  __X, __W );
            __W->scale( -1. );
            //std::cout << "__W->norm=" << __W->l2Norm() << "\n";
            M_model->l2solve( __Z1, __W );

            for ( int __q2 = 0; __q2 < __QRhs; ++__q2 )
            {
                M_Lambda_pr[__q1][__q2].conservativeResize( __N );

                //__Y = Fq[0][__q2];
                //std::cout << "__Fq->norm=" << Fq[0][__q2]->l2Norm() << "\n";
                M_model->l2solve( __Z2, Fq[0][__q2] );
                M_Lambda_pr[ __q1][ __q2]( elem ) = 2.0*M_model->scalarProduct( __Z1, __Z2 );
                //VLOG(1) << "M_Lambda_pr[" << __q1 << "][" << __q2 << "][" << __j << "]=" << M_Lambda_pr[__q1][__q2][__j] << "\n";
                //std::cout << "M_Lambda_pr[" << __q1 << "][" << __q2 << "][" << __j << "]=" << M_Lambda_pr[__q1][__q2][__j] << "\n";
            }
        }
    }


    std::cout << "     o Lambda_pr updated in " << ti.elapsed() << "s\n";
    ti.restart();

    for ( int __q1 = 0; __q1 < __QLhs; ++__q1 )
    {
        for ( int __q2 = 0; __q2 < __QLhs; ++__q2 )
        {
            M_Gamma_pr[__q1][__q2].conservativeResize( __N, __N );

            for ( int elem=__N-number_of_added_elements; elem<__N; elem++ )
            {
                *__X=M_WN[elem];
                Aq[__q1]->multVector(  __X, __W );
                __W->scale( -1. );
                //std::cout << "__W->norm=" << __W->l2Norm() << "\n";
                M_model->l2solve( __Z1, __W );

                //Aq[__q2]->multVector(  __Z1, __W );
                for ( int __l = 0; __l < ( int )__N; ++__l )
                {
                    *__X=M_WN[__l];
                    Aq[__q2]->multVector(  __X, __W );
                    __W->scale( -1. );
                    //std::cout << "__W2_pr->norm=" << __W->l2Norm() << "\n";
                    M_model->l2solve( __Z2, __W );
                    M_Gamma_pr[ __q1][ __q2]( elem,__l ) = M_model->scalarProduct( __Z1, __Z2 );
                    //M_Gamma_pr[ __q2][ __q1][ __j ][__l] = M_Gamma_pr[ __q1][ __q2][ __j ][__l];
                    //VLOG(1) << "M_Gamma_pr[" << __q1 << "][" << __q2 << "][" << __j << "][" << __l << "]=" << M_Gamma_pr[__q1][__q2][__j][__l] << "\n";
                    //std::cout << "M_Gamma_pr[" << __q1 << "][" << __q2 << "][" << __j << "][" << __l << "]=" << M_Gamma_pr[__q1][__q2][__j][__l] << "\n";
                }
            }

            for ( int __j = 0; __j < ( int )__N; ++__j )
            {
                *__X=M_WN[__j];
                Aq[__q1]->multVector(  __X, __W );
                __W->scale( -1. );
                //std::cout << "__W->norm=" << __W->l2Norm() << "\n";
                M_model->l2solve( __Z1, __W );

                //Aq[__q2]->multVector(  __Z1, __W );
                //column N-1
                //int __l = __N-1;
                for ( int elem=__N-number_of_added_elements; elem<__N; elem++ )
                {
                    *__X=M_WN[elem];
                    Aq[__q2]->multVector(  __X, __W );
                    __W->scale( -1. );
                    //std::cout << "__W2_pr->norm=" << __W->l2Norm() << "\n";
                    M_model->l2solve( __Z2, __W );
                    M_Gamma_pr[ __q1][ __q2]( __j,elem ) = M_model->scalarProduct( __Z1, __Z2 );
                    //M_Gamma_pr[ __q2][ __q1][ __j ][__l] = M_Gamma_pr[ __q1][ __q2][ __j ][__l];
                    //VLOG(1) << "M_Gamma_pr[" << __q1 << "][" << __q2 << "][" << __j << "][" << __l << "]=" << M_Gamma_pr[__q1][__q2][__j][__l] << "\n";
                    //std::cout << "M_Gamma_pr[" << __q1 << "][" << __q2 << "][" << __j << "][" << __l << "]=" << M_Gamma_pr[__q1][__q2][__j][__l] << "\n";
                }
            }

        }// on N1
    } // on q1

    std::cout << "     o Gamma_pr updated in " << ti.elapsed() << "s\n";
    ti.restart();
    sparse_matrix_ptrtype Atq1 = M_model->newMatrix();
    sparse_matrix_ptrtype Atq2 = M_model->newMatrix();

    //
    // Dual
    //
    // compute this only once
    if ( Ncur == 1 )
    {
        LOG(INFO) << "[offlineResidual] Compute Dual residual data\n";
        LOG(INFO) << "[offlineResidual] C0_du\n";

        for ( int __q1 = 0; __q1 < __QOutput; ++__q1 )
        {
            *__Fdu = *Fq[M_output_index][__q1];
            __Fdu->scale( -1.0 );
            M_model->l2solve( __Z1, __Fdu );

            for ( int __q2 = 0; __q2 < __QOutput; ++__q2 )
            {
                *__Fdu = *Fq[M_output_index][__q2];
                __Fdu->scale( -1.0 );
                M_model->l2solve( __Z2, __Fdu );
                M_C0_du[__q1][__q2] = M_model->scalarProduct( __Z1, __Z2 );
                //M_C0_du[__q2][__q1] = M_C0_du[__q1][__q2];
            }

            //M_C0_du[__q1][__q1] = M_model->scalarProduct( __Xdu, __Xdu );
        }

        std::cout << "     o C0_du updated in " << ti.elapsed() << "s\n";
        ti.restart();
    }

    LOG(INFO) << "[offlineResidual] Lambda_du, Gamma_du\n";

    for ( int elem=__N-number_of_added_elements; elem<__N; elem++ )
    {
        *__X=M_WNdu[elem];

        for ( int __q1 = 0; __q1 < __QLhs; ++__q1 )
        {
            Aq[__q1]->transpose( Atq1 );
            Atq1->multVector(  __X, __W );
            __W->scale( -1. );
            //std::cout << "__W->norm=" << __W->l2Norm() << "\n";
            M_model->l2solve( __Z1, __W );

            for ( int __q2 = 0; __q2 < __QOutput; ++__q2 )
            {
                M_Lambda_du[__q1][__q2].conservativeResize( __N );

                *__Fdu = *Fq[M_output_index][__q2];
                __Fdu->scale( -1.0 );
                M_model->l2solve( __Z2, __Fdu );
                M_Lambda_du[ __q1][ __q2]( elem ) = 2.0*M_model->scalarProduct( __Z2, __Z1 );
                //VLOG(1) << "M_Lambda_pr[" << __q1 << "][" << __q2 << "][" << __j << "]=" << M_Lambda_pr[__q1][__q2][__j] << "\n";
                //std::cout << "M_Lambda_pr[" << __q1 << "][" << __q2 << "][" << __j << "]=" << M_Lambda_pr[__q1][__q2][__j] << "\n";
            } // q2
        } // q2
    }

    std::cout << "     o Lambda_du updated in " << ti.elapsed() << "s\n";
    ti.restart();

    for ( int __q1 = 0; __q1 < __QLhs; ++__q1 )
    {
        Aq[__q1]->transpose( Atq1 );

        for ( int __q2 = 0; __q2 < __QLhs; ++__q2 )
        {
            Aq[__q2]->transpose( Atq2 );
            M_Gamma_du[__q1][__q2].conservativeResize( __N, __N );

            for ( int elem=__N-number_of_added_elements; elem<__N; elem++ )
            {
                //int __j = __N-1;
                *__X=M_WNdu[elem];
                Atq1->multVector(  __X, __W );
                __W->scale( -1. );
                //std::cout << "__W->norm=" << __W->l2Norm() << "\n";
                M_model->l2solve( __Z1, __W );

                //Aq[__q2]->multVector(  __Z1, __W );

                for ( int __l = 0; __l < ( int )__N; ++__l )
                {
                    *__X=M_WNdu[__l];
                    Atq2->multVector(  __X, __W );
                    __W->scale( -1. );
                    //std::cout << "__W2_pr->norm=" << __W->l2Norm() << "\n";
                    M_model->l2solve( __Z2, __W );
                    M_Gamma_du[ __q1][ __q2]( elem,__l ) = M_model->scalarProduct( __Z1, __Z2 );
                    //M_Gamma_pr[ __q2][ __q1][ __j ][__l] = M_Gamma_pr[ __q1][ __q2][ __j ][__l];
                    //VLOG(1) << "M_Gamma_pr[" << __q1 << "][" << __q2 << "][" << __j << "][" << __l << "]=" << M_Gamma_pr[__q1][__q2][__j][__l] << "\n";
                    //std::cout << "M_Gamma_pr[" << __q1 << "][" << __q2 << "][" << __j << "][" << __l << "]=" << M_Gamma_pr[__q1][__q2][__j][__l] << "\n";
                }
            }

            // update column __N-1
            for ( int __j = 0; __j < ( int )__N; ++__j )
            {
                *__X=M_WNdu[__j];
                Atq1->multVector(  __X, __W );
                __W->scale( -1. );
                //std::cout << "__W->norm=" << __W->l2Norm() << "\n";
                M_model->l2solve( __Z1, __W );

                //Aq[__q2]->multVector(  __Z1, __W );
                //int __l = __N-1;
                for ( int elem=__N-number_of_added_elements; elem<__N; elem++ )
                {
                    *__X=M_WNdu[elem];
                    Atq2->multVector(  __X, __W );
                    __W->scale( -1. );
                    //std::cout << "__W2_pr->norm=" << __W->l2Norm() << "\n";
                    M_model->l2solve( __Z2, __W );
                    M_Gamma_du[ __q1][ __q2]( __j,elem ) = M_model->scalarProduct( __Z1, __Z2 );
                    //M_Gamma_pr[ __q2][ __q1][ __j ][__l] = M_Gamma_pr[ __q1][ __q2][ __j ][__l];
                    //VLOG(1) << "M_Gamma_pr[" << __q1 << "][" << __q2 << "][" << __j << "][" << __l << "]=" << M_Gamma_pr[__q1][__q2][__j][__l] << "\n";
                    //std::cout << "M_Gamma_pr[" << __q1 << "][" << __q2 << "][" << __j << "][" << __l << "]=" << M_Gamma_pr[__q1][__q2][__j][__l] << "\n";
                }
            }

        }// on N1
    } // on q1

    std::cout << "     o Gamma_du updated in " << ti.elapsed() << "s\n";
    ti.restart();
    LOG(INFO) << "[offlineResidual] Done.\n";

}



template<typename TruthModelType>
void
CRB_heatns<TruthModelType>::printMuSelection( void )
{
    LOG(INFO)<<" List of parameter selectionned during the offline algorithm \n";
    for(int k=0;k<M_WNmu->size();k++)
    {
	std::cout<<" mu "<<k<<" = [ ";
	LOG(INFO)<<" mu "<<k<<" = [ ";
	parameter_type const& _mu = M_WNmu->at( k );
	for( int i=0; i<_mu.size()-1; i++ )
	{
	    LOG(INFO)<<_mu(i)<<" , ";
	    std::cout<<_mu(i)<<" , ";
	}
	LOG(INFO)<<_mu( _mu.size()-1 )<<" ] \n";
	std::cout<<_mu( _mu.size()-1 )<<" ] "<<std::endl;
    }
}



template<typename TruthModelType>
boost::tuple<double,double,double,double>
CRB_heatns<TruthModelType>::run( parameter_type const& mu, double eps )
{

    M_compute_variance = this->vm()["crb.compute-variance"].template as<bool>();
    //int Nwn = M_N;
    int Nwn = vm()["crb.dimension-max"].template as<int>();
#if 0

    if (  M_error_type!=CRB_EMPIRICAL )
    {
        auto lo = M_rbconv.right.range( boost::bimaps::unbounded, boost::bimaps::_key <= eps );

        for ( auto it = lo.first; it != lo.second; ++it )
        {
            std::cout << "rbconv[" << it->first <<"]=" << it->second << "\n";
        }

        auto it = M_rbconv.project_left( lo.first );
        Nwn = it->first;
        std::cout << "Nwn = "<< Nwn << " error = "<< it->second.template get<0>() << " eps=" << eps << "\n";
    }

#endif


    std::vector<vectorN_type> uN;
    std::vector<vectorN_type> uNdu;
    std::vector<vectorN_type> uNold;
    std::vector<vectorN_type> uNduold;

    auto o = lb( Nwn, mu, uN, uNdu , uNold, uNduold );
    double output = o.template get<0>();

    auto error_estimation = delta( Nwn, mu, uN, uNdu , uNold, uNduold );

    double e = error_estimation.template get<0>();
    double condition_number = o.template get<1>();

    return boost::make_tuple( output , e, Nwn , condition_number );
}


template<typename TruthModelType>
void
CRB_heatns<TruthModelType>::run( const double * X, unsigned long N, double * Y, unsigned long P)
{


    parameter_type mu( M_Dmu );

    for ( unsigned long p= 0; p < N-5; ++p )
      {
        mu( p ) = X[p];
	std::cout << "mu( " << p << " ) = " << mu( p ) << std::endl;
      }


    std::cout<<" list of parameters : [";

    for ( unsigned long i=0; i<N-1; i++ ) std::cout<<X[i]<<" , ";

    std::cout<<X[N-1]<<" ] "<<std::endl;


    setOutputIndex( ( int )X[N-5] );
    std::cout<<"output_index = "<<X[N-5]<<std::endl;
    int Nwn =  X[N-4];
    std::cout<<" Nwn = "<<Nwn<<std::endl;
    int maxerror = X[N-3];
    std::cout<<" maxerror = "<<maxerror<<std::endl;
    CRBErrorType_heatns errorType =( CRBErrorType_heatns )X[N-2];
    std::cout<<"errorType = "<<X[N-2]<<std::endl;
    setCRBErrorType( errorType );
    M_compute_variance = X[N-1];
    std::cout<<"M_compute_variance = "<<M_compute_variance<<std::endl;

#if 0
    auto lo = M_rbconv.right.range( boost::bimaps::unbounded,boost::bimaps::_key <= maxerror );


    for ( auto it = lo.first; it != lo.second; ++it )
    {
        std::cout << "rbconv[" << it->first <<"]=" << it->second << "\n";
    }

    auto it = M_rbconv.project_left( lo.first );
    Nwn = it->first;
    //std::cout << "Nwn = "<< Nwn << " error = "<< it->second << " maxerror=" << maxerror << " ErrorType = "<<errorType <<"\n";
#endif
    //int Nwn = M_WN.size();
    std::vector<vectorN_type> uN;
    std::vector<vectorN_type> uNdu;
    std::vector<vectorN_type> uNold ;
    std::vector<vectorN_type> uNduold;

    FEELPP_ASSERT( P == 2 )( P ).warn( "invalid number of outputs" );
    auto o = lb( Nwn, mu, uN, uNdu , uNold, uNduold );
    auto e = delta( Nwn, mu, uN, uNdu , uNold, uNduold );
    Y[0]  = o.template get<0>();
    Y[1]  = e.template get<0>();
}

template<typename TruthModelType>
template<class Archive>
void
CRB_heatns<TruthModelType>::save( Archive & ar, const unsigned int version ) const
{

    std::cout<<"[CRB::save] version : "<<version<<std::endl;

    auto mesh = mesh_type::New();
    auto is_mesh_loaded = mesh->load( _name="mymesh",_path=".",_type="binary" );
    if ( ! is_mesh_loaded )
    {
        auto first_element = M_WN[0];
        mesh = first_element.functionSpace()->mesh() ;
        mesh->save( _name="mymesh",_path=".",_type="binary" );
    }

    auto Xh = space_type::New( mesh );

    ar & boost::serialization::base_object<super>( *this );
    ar & BOOST_SERIALIZATION_NVP( M_output_index );
    ar & BOOST_SERIALIZATION_NVP( M_N );
    ar & BOOST_SERIALIZATION_NVP( M_rbconv );
    ar & BOOST_SERIALIZATION_NVP( M_error_type );
    ar & BOOST_SERIALIZATION_NVP( M_Xi );
    ar & BOOST_SERIALIZATION_NVP( M_WNmu );
    ar & BOOST_SERIALIZATION_NVP( M_Aq_pr );
    ar & BOOST_SERIALIZATION_NVP( M_Aq_du );
    ar & BOOST_SERIALIZATION_NVP( M_Aq_pr_du );
    ar & BOOST_SERIALIZATION_NVP( M_Fq_pr );
    ar & BOOST_SERIALIZATION_NVP( M_Fq_du );
    ar & BOOST_SERIALIZATION_NVP( M_Lq_pr );
    ar & BOOST_SERIALIZATION_NVP( M_Lq_du );

    ar & BOOST_SERIALIZATION_NVP( M_C0_pr );
    ar & BOOST_SERIALIZATION_NVP( M_C0_du );
    ar & BOOST_SERIALIZATION_NVP( M_Lambda_pr );
    ar & BOOST_SERIALIZATION_NVP( M_Lambda_du );
    ar & BOOST_SERIALIZATION_NVP( M_Gamma_pr );
    ar & BOOST_SERIALIZATION_NVP( M_Gamma_du );


    if ( model_type::is_time_dependent )
    {
        ar & BOOST_SERIALIZATION_NVP( M_Mq_pr );
        ar & BOOST_SERIALIZATION_NVP( M_Mq_du );
        ar & BOOST_SERIALIZATION_NVP( M_Mq_pr_du );

        if ( version>=1 )
        {
            ar & BOOST_SERIALIZATION_NVP( M_coeff_pr_ini_online );
            ar & BOOST_SERIALIZATION_NVP( M_coeff_du_ini_online );
            ar & BOOST_SERIALIZATION_NVP( M_Cmf_pr );
            ar & BOOST_SERIALIZATION_NVP( M_Cma_pr );
            ar & BOOST_SERIALIZATION_NVP( M_Cmm_pr );
            ar & BOOST_SERIALIZATION_NVP( M_Cmf_du );
            ar & BOOST_SERIALIZATION_NVP( M_Cma_du );
            ar & BOOST_SERIALIZATION_NVP( M_Cmm_du );
        }
    }
    if( version >= 2 )
        ar & BOOST_SERIALIZATION_NVP( M_variance_matrix_phi );


    if( version >= 4 )
    {
        ar & BOOST_SERIALIZATION_NVP( M_current_mu );

        for(int i=0; i<M_N; i++)
            ar & BOOST_SERIALIZATION_NVP( M_WN[i] );
        for(int i=0; i<M_N; i++)
            ar & BOOST_SERIALIZATION_NVP( M_WNdu[i] );
    }
}

template<typename TruthModelType>
template<class Archive>
void
CRB_heatns<TruthModelType>::load( Archive & ar, const unsigned int version )
{

    std::cout<<"[CRB::load] version"<< version <<std::endl;

    auto mesh = mesh_type::New();
    auto is_mesh_loaded = mesh->load( _name="mymesh",_path=".",_type="binary" );

    auto Xh = space_type::New( mesh );

    if( version <= 2 )
        M_rbconv_contains_primal_and_dual_contributions = false;
    else
        M_rbconv_contains_primal_and_dual_contributions = true;


    typedef boost::bimap< int, double > old_convergence_type;
    ar & boost::serialization::base_object<super>( *this );
    ar & BOOST_SERIALIZATION_NVP( M_output_index );
    ar & BOOST_SERIALIZATION_NVP( M_N );

   if( version <= 2 )
    {
        old_convergence_type old_M_rbconv;
        ar & BOOST_SERIALIZATION_NVP( old_M_rbconv );
        double delta_pr = 0;
        double delta_du = 0;
        typedef old_convergence_type::left_map::const_iterator iterator;
        for(iterator it = old_M_rbconv.left.begin(); it != old_M_rbconv.left.end(); ++it)
        {
            int N = it->first;
            double maxerror = it->second;
            M_rbconv.insert( convergence( N, boost::make_tuple(maxerror,delta_pr,delta_du) ) );
        }
    }
   else
	ar & BOOST_SERIALIZATION_NVP( M_rbconv );

    ar & BOOST_SERIALIZATION_NVP( M_error_type );
    ar & BOOST_SERIALIZATION_NVP( M_Xi );
    ar & BOOST_SERIALIZATION_NVP( M_WNmu );
    ar & BOOST_SERIALIZATION_NVP( M_Aq_pr );
    ar & BOOST_SERIALIZATION_NVP( M_Aq_du );
    ar & BOOST_SERIALIZATION_NVP( M_Aq_pr_du );
    ar & BOOST_SERIALIZATION_NVP( M_Fq_pr );
    ar & BOOST_SERIALIZATION_NVP( M_Fq_du );
    ar & BOOST_SERIALIZATION_NVP( M_Lq_pr );
    ar & BOOST_SERIALIZATION_NVP( M_Lq_du );
    ar & BOOST_SERIALIZATION_NVP( M_C0_pr );
    ar & BOOST_SERIALIZATION_NVP( M_C0_du );
    ar & BOOST_SERIALIZATION_NVP( M_Lambda_pr );
    ar & BOOST_SERIALIZATION_NVP( M_Lambda_du );
    ar & BOOST_SERIALIZATION_NVP( M_Gamma_pr );
    ar & BOOST_SERIALIZATION_NVP( M_Gamma_du );
    if ( model_type::is_time_dependent )
    {
        ar & BOOST_SERIALIZATION_NVP( M_Mq_pr );
        ar & BOOST_SERIALIZATION_NVP( M_Mq_du );
        ar & BOOST_SERIALIZATION_NVP( M_Mq_pr_du );

        if ( version>=1 )
        {
            ar & BOOST_SERIALIZATION_NVP( M_coeff_pr_ini_online );
            ar & BOOST_SERIALIZATION_NVP( M_coeff_du_ini_online );
            ar & BOOST_SERIALIZATION_NVP( M_Cmf_pr );
            ar & BOOST_SERIALIZATION_NVP( M_Cma_pr );
            ar & BOOST_SERIALIZATION_NVP( M_Cmm_pr );
            ar & BOOST_SERIALIZATION_NVP( M_Cmf_du );
            ar & BOOST_SERIALIZATION_NVP( M_Cma_du );
            ar & BOOST_SERIALIZATION_NVP( M_Cmm_du );
        }
    }
    if( version >= 2 )
        ar & BOOST_SERIALIZATION_NVP( M_variance_matrix_phi );

    if( version >= 4 )
    {
        ar & BOOST_SERIALIZATION_NVP( M_current_mu );

        element_type temp = Xh->element();
        M_WN.resize( M_N );
        M_WNdu.resize( M_N );
        temp = Xh->element();
        for( int i = 0 ; i < M_N ; i++ )
        {
            temp.setName( (boost::format( "fem-primal-%1%" ) % ( i ) ).str() );
            ar & BOOST_SERIALIZATION_NVP( temp );
            M_WN[i] = temp;
        }
        for( int i = 0 ; i < M_N ; i++ )
        {
            temp.setName( (boost::format( "fem-dual-%1%" ) % ( i ) ).str() );
            ar & BOOST_SERIALIZATION_NVP( temp );
            M_WNdu[i] = temp;
        }
    }

#if 0
    std::cout << "[loadDB] output index : " << M_output_index << "\n"
              << "[loadDB] N : " << M_N << "\n"
              << "[loadDB] error type : " << M_error_type << "\n";

    for ( auto it = M_rbconv.begin(), en = M_rbconv.end();
            it != en; ++it )
    {
        std::cout << "[loadDB] convergence: (" << it->left << ","  << it->right  << ")\n";
    }

#endif

}


template<typename TruthModelType>
bool
CRB_heatns<TruthModelType>::printErrorDuringOfflineStep()
{
  bool print = this->vm()["crb.print-error-during-rb-construction"].template as<bool>();
    return print;
}

template<typename TruthModelType>
bool
CRB_heatns<TruthModelType>::rebuildDB()
{
    bool rebuild = this->vm()["crb.rebuild-database"].template as<bool>();
    return rebuild;
}

template<typename TruthModelType>
bool
CRB_heatns<TruthModelType>::showMuSelection()
{
    bool show = this->vm()["crb.show-mu-selection"].template as<bool>();
    return show;
}

template<typename TruthModelType>
void
CRB_heatns<TruthModelType>::saveDB()
{
    fs::ofstream ofs( this->dbLocalPath() / this->dbFilename() );

    if ( ofs )
    {
        boost::archive::text_oarchive oa( ofs );
        // write class instance to archive
        oa << *this;
        // archive and stream closed when destructors are called
    }
}

template<typename TruthModelType>
bool
CRB_heatns<TruthModelType>::loadDB()
{
    if ( this->rebuildDB() )
        return false;

    fs::path db = this->lookForDB();

    if ( db.empty() )
        return false;

    if ( !fs::exists( db ) )
        return false;

    //std::cout << "Loading " << db << "...\n";
    fs::ifstream ifs( db );

    if ( ifs )
    {
        boost::archive::text_iarchive ia( ifs );
        // write class instance to archive
        ia >> *this;
        //std::cout << "Loading " << db << " done...\n";
        this->setIsLoaded( true );
        // archive and stream closed when destructors are called
        return true;
    }

    return false;
}
} // Feel

namespace boost
{
namespace serialization
{
template< typename T>
struct version< Feel::CRB_heatns<T> >
{
    // at the moment the version of the CRB DB is 4. if any changes is done
    // to the format it is mandatory to increase the version number below
    // and use the new version number of identify the new entries in the DB
    typedef mpl::int_<4> type;
    typedef mpl::integral_c_tag tag;
    static const unsigned int value = version::type::value;
};
template<typename T> const unsigned int version<Feel::CRB_heatns<T> >::value;
}
}
#endif /* __CRB_heatns_H */