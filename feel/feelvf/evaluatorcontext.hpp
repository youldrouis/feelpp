/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t -*-

  This file is part of the Feel library

  Author(s): Christophe Prud'homme <christophe.prudhomme@feelpp.org>
       Date: 2012-03-20

  Copyright (C) 2013 Feel++ Consortium

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
   \file evaluator.hpp
   \author Christophe Prud'homme <christophe.prudhomme@feelpp.org>
   \date 2013-03-20
 */
#ifndef __FEELPP_EVALUATORCONTEXT_H
#define __FEELPP_EVALUATORCONTEXT_H 1

#include <boost/timer.hpp>
#include <boost/signal.hpp>
#include <feel/feelcore/parameter.hpp>
#include <feel/feeldiscr/functionspace.hpp>

namespace Feel
{
namespace vf
{
namespace details
{
/**
 * \class EvaluatorContext
 * \brief work class to evaluate expressions at sets of points
 *
 * @author Christophe Prud'homme
 */
template<typename CTX, typename ExprT>
class EvaluatorContext
{
public:


    /** @name Typedefs
     */
    //@{

    static const size_type context = ExprT::context|vm::POINT;

    typedef ExprT expression_type;
    typedef typename expression_type::value_type value_type;


    static const uint16_type imorder = 1;
    static const bool imIsPoly = true;

    typedef CTX context_type;
    typedef Eigen::Matrix<value_type,Eigen::Dynamic,1> element_type;

    //@}

    /** @name Constructors, destructor
     */
    //@{

    EvaluatorContext( context_type const& ctx,
                      expression_type const& __expr,
                      GeomapStrategyType geomap_strategy )
        :
        M_ctx( ctx ),
        M_expr( __expr ),
        M_geomap_strategy( geomap_strategy )
    {
        DVLOG(2) << "EvaluatorContext constructor from expression\n";
    }


    EvaluatorContext( EvaluatorContext const& __vfi )
        :
        M_ctx( __vfi.M_ctx ),
        M_expr( __vfi.M_expr ),
        M_geomap_strategy( __vfi.M_geomap_strategy )
    {
        DVLOG(2) << "EvaluatorContext copy constructor\n";
    }

    virtual ~EvaluatorContext() {}

    //@}

    /** @name Operator overloads
     */
    //@{

    element_type operator()() const;


    //@}

    /** @name Accessors
     */
    //@{

    /**
     * get the variational expression
     *
     *
     * @return the variational expression
     */
    expression_type const& expression() const
    {
        return M_expr;
    }

    //@}

    /** @name  Mutators
     */
    //@{

    //@}

    /** @name  Methods
     */
    //@{

    //@}

private:

    context_type M_ctx;
    expression_type const&  M_expr;
    GeomapStrategyType M_geomap_strategy;
};

template<typename CTX, typename ExprT>
typename EvaluatorContext<CTX, ExprT>::element_type
EvaluatorContext<CTX, ExprT>::operator()() const
{
    //boost::timer __timer;

    //rank of the current processor
    int proc_number = Environment::worldComm().globalRank();

    //total number of processors
    int nprocs = Environment::worldComm().globalSize();

    auto it = M_ctx.begin();
    auto en = M_ctx.end();

    typedef typename CTX::value_type::element_type::geometric_mapping_context_ptrtype gm_context_ptrtype;
    typedef fusion::map<fusion::pair<vf::detail::gmc<0>, gm_context_ptrtype> > map_gmc_type;
    typedef expression_type the_expression_type;
    typedef typename boost::remove_reference<typename boost::remove_const<the_expression_type>::type >::type iso_expression_type;
    typedef typename iso_expression_type::template tensor<map_gmc_type> t_expr_type;
    typedef typename t_expr_type::value_type value_type;
    typedef typename t_expr_type::shape shape;

    CHECK( shape::M == 1 ) << "Invalid expression shape " << shape::M << " should be 1";

    int npoints = M_ctx.nPoints();

    element_type __v( npoints*shape::M );
    __v.setZero();

    //local version of __v on each proc
    element_type __localv( M_ctx.size()*shape::M );
    __localv.setZero();


    if ( !M_ctx.empty() )
    {

        for ( int p = 0; it!=en ; ++it, ++p )
        {
            auto const& ctx = *it;

            /**
             * be careful there is no guarantee that the set of contexts will
             * have the reference points. We should probably have a flag set by
             * the programmer so that we don't have to re-create the expression
             * context if the reference points are the same
             */
            map_gmc_type mapgmci( fusion::make_pair<vf::detail::gmc<0> >(ctx->gmContext() ) );
            t_expr_type tensor_expr( M_expr, mapgmci );
            tensor_expr.update( mapgmci );

            for ( uint16_type c1 = 0; c1 < shape::M; ++c1 )
            {
                __localv( shape::M*p+c1) = tensor_expr.evalq( c1, 0, 0 );
            }
        }
    }

    //now every proc have filled localv ( if they have points )
    //each proc has to fill __v
    if( nprocs > 1 )
    {
        int index=0;//the proc may contains severals points se we need to have
                    //an index to insert values

        for(int p=0; p<npoints; p++)
        {
            int proc_having_point = M_ctx.processorHavingPoint(p);
            if( proc_number == proc_having_point )
            {
                boost::mpi::broadcast( Environment::worldComm() , __localv(index) , proc_number );
                __v( p ) = __localv( index );
                index++;//local index increases
            }
            else
            {
                double contribution_from_other ;
                boost::mpi::broadcast( Environment::worldComm() , contribution_from_other , proc_having_point );
                __v( p ) = contribution_from_other ;
            }
        }
    }// nprocs > 1
    else
        __v = __localv;

    return __v;
}

}
/// \endcond

/// \cond DETAIL
namespace detail
{
template<typename Args>
struct evaluate_context
{
    typedef typename clean_type<Args,tag::expr>::type _expr_type;
    typedef typename clean_type<Args,tag::context>::type _context_type;
    typedef details::EvaluatorContext<_context_type, Expr<_expr_type> > eval_t;
    typedef typename eval_t::element_type element_type;
};
}
/// \endcond


template<typename Ctx, typename ExprT>
typename details::EvaluatorContext<Ctx, Expr<ExprT> >::element_type
evaluatecontext_impl( Ctx const& ctx,
                      Expr<ExprT> const& __expr,
                      GeomapStrategyType geomap = GeomapStrategyType::GEOMAP_HO )
{
    typedef details::EvaluatorContext<Ctx, Expr<ExprT> > proj_t;
    proj_t p( ctx, __expr, geomap );
    return p();
}


/**
 *
 * \brief evaluate an expression over a range of element at a set of points defined in the reference element
 *
 * \arg range the range of mesh elements to apply the projection (the remaining parts are set to 0)
 * \arg pset set of points (e.g. quadrature points) in the reference elements to be transformed in the real elements
 * \arg expr the expression to project
 * \arg geomap the type of geomap to use (make sense only using high order meshes)
 */
BOOST_PARAMETER_FUNCTION(
    ( typename vf::detail::evaluate_context<Args>::element_type ), // return type
    evaluateFromContext,    // 2. function name

    tag,           // 3. namespace of tag types

    ( required
      ( context, *  )
      ( expr, * )
    ) // 4. one required parameter, and

    ( optional
      ( geomap,         *, GeomapStrategyType::GEOMAP_OPT )
    )
)
{
    LOG(INFO) << "evaluate expression..." << std::endl;
    return evaluatecontext_impl( context, expr, geomap );
    LOG(INFO) << "evaluate expression done." << std::endl;
}


} // vf
} // feel


#endif /* __FEELPP_EVALUATORS_H */

