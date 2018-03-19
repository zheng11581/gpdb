//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CPhysicalJoin.h
//
//	@doc:
//		Physical join base class
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalJoin_H
#define GPOPT_CPhysicalJoin_H

#include "gpos/base.h"

#include "gpopt/base/CDistributionSpec.h"
#include "gpopt/operators/CPhysical.h"

namespace gpopt
{
	
	// fwd declarations
	class CDistributionSpecSingleton;

	//---------------------------------------------------------------------------
	//	@class:
	//		CPhysicalJoin
	//
	//	@doc:
	//		Inner nested-loops join operator
	//
	//---------------------------------------------------------------------------
	class CPhysicalJoin : public CPhysical
	{

		private:

			// private copy ctor
			CPhysicalJoin(const CPhysicalJoin &);

			// check whether the child being processed is the child that has the part consumer
			static
			BOOL FProcessingChildWithPartConsumer
				(
				BOOL fOuterPartConsumerTest,
				ULONG ulChildIndexToTestFirst,
				ULONG ulChildIndexToTestSecond,
				ULONG ulChildIndex
				);

		protected:
		
			// Partition propagation request generated by a join
			class CPartPropReq : public CRefCount
			{
				private:

					// required part propagation
					CPartitionPropagationSpec *m_pppsRequired;

					// child index to push requirements to
					ULONG m_ulChildIndex;

					// id of outer child
					ULONG m_ulOuterChild;

					// id of inner child
					ULONG m_ulInnerChild;

					// id of scalar child
					ULONG m_ulScalarChild;

					// private copy ctor
					CPartPropReq(const CPartPropReq&);

				public:

					// ctor
					CPartPropReq
						(
						CPartitionPropagationSpec *pppsRequired,
						ULONG ulChildIndex,
						ULONG ulOuterChild,
						ULONG ulInnerChild,
						ULONG ulScalarChild
						)
						:
						m_pppsRequired(pppsRequired),
						m_ulChildIndex(ulChildIndex),
						m_ulOuterChild(ulOuterChild),
						m_ulInnerChild(ulInnerChild),
						m_ulScalarChild(ulScalarChild)
					{
						GPOS_ASSERT(NULL != pppsRequired);
					}

					// dtor
					virtual
					~CPartPropReq()
					{
						m_pppsRequired->Release();
					}

					// partition propgataion spec
					CPartitionPropagationSpec *Ppps() const
					{
						return m_pppsRequired;
					}

					// child index to push requirements to
					ULONG UlChildIndex() const
					{
						return m_ulChildIndex;
					}

					// outer child id
					ULONG UlOuterChild() const
					{
						return m_ulOuterChild;
					}

					// inner child id
					ULONG UlInnerChild() const
					{
						return m_ulInnerChild;
					}

					// scalar child id
					ULONG UlScalarChild() const
					{
						return m_ulScalarChild;
					}

					// hash function
					static
					ULONG UlHash(const CPartPropReq *pppr);

					// equality function
					static
					BOOL FEqual(const CPartPropReq *ppprFst, const CPartPropReq *ppprSnd);

			}; // class CPartPropReq

			// map partition propagation request to partition propagation spec
			typedef CHashMap<CPartPropReq, CPartitionPropagationSpec, CPartPropReq::UlHash, CPartPropReq::FEqual,
						CleanupRelease<CPartPropReq>, CleanupRelease<CPartitionPropagationSpec> > HMPartPropagation;

			// mutex for locking map of child columns requests during lookup/insertion
			CMutex m_mutexJoin;

			// partition propagation request map
			HMPartPropagation *m_phmpp;

			// ctor
			explicit
			CPhysicalJoin(IMemoryPool *pmp);

			// dtor
			virtual 
			~CPhysicalJoin();

			// helper to check if given child index correspond to first child to be optimized
			BOOL FFirstChildToOptimize(ULONG ulChildIndex) const;

			// helper to compute required distribution of correlated join's children
			CDistributionSpec *PdsRequiredCorrelatedJoin
				(
				IMemoryPool *pmp,
				CExpressionHandle &exprhdl,
				CDistributionSpec *pdsRequired,
				ULONG ulChildIndex,
				DrgPdp *pdrgpdpCtxt,
				ULONG  ulOptReq
				)
				const;

			// helper to compute required rewindability of correlated join's children
			CRewindabilitySpec *PrsRequiredCorrelatedJoin
				(
				IMemoryPool *pmp,
				CExpressionHandle &exprhdl,
				CRewindabilitySpec *prsRequired,
				ULONG ulChildIndex,
				DrgPdp *pdrgpdpCtxt,
				ULONG ulOptReq
				)
				const;

			// create partition propagation request
			CPartPropReq *PpprCreate
				(
				IMemoryPool *pmp,
				CExpressionHandle &exprhdl,
				CPartitionPropagationSpec *pppsRequired,
				ULONG ulChildIndex
				);

			// compute required partition propagation of the n-th child
			CPartitionPropagationSpec *PppsRequiredCompute
				(
				IMemoryPool *pmp,
				CExpressionHandle &exprhdl,
				CPartitionPropagationSpec *pppsRequired,
				ULONG ulChildIndex,
				BOOL fNLJoin
				);

			// helper function for computing the required partition propagation
			// spec for the children of a join
			CPartitionPropagationSpec *PppsRequiredJoinChild
				(
				IMemoryPool *pmp,
				CExpressionHandle &exprhdl,
				CPartitionPropagationSpec *pppsRequired,
				ULONG ulChildIndex,
				DrgPdp *pdrgpdpCtxt,
				BOOL fNLJoin
				);

			// helper for propagating required sort order to outer child
			static
			COrderSpec *PosPropagateToOuter(IMemoryPool *pmp, CExpressionHandle &exprhdl, COrderSpec *posRequired);

			// helper for checking if required sort columns come from outer child
			static
			BOOL FSortColsInOuterChild(IMemoryPool *pmp, CExpressionHandle &exprhdl, COrderSpec *pos);

			// helper for checking if the outer input of a binary join operator
			// includes the required columns
			static
			BOOL FOuterProvidesReqdCols(CExpressionHandle &exprhdl, CColRefSet *pcrsRequired);

			// helper for adding a pair of hash join keys to given arrays
			static
			void AddHashKeys
				(
				CExpression *pexprPred,
				CExpression *pexprOuter,
				CExpression *pexprInner,
				DrgPexpr *pdrgpexprOuter,
				DrgPexpr *pdrgpexprInner
				);
			
			// helper to add filter on part key
			static
			void AddFilterOnPartKey
				(
				IMemoryPool *pmp,
				BOOL fNLJoin,
				CExpression *pexprScalar,
				CPartIndexMap *ppimSource,
				CPartFilterMap *ppfmSource,
				ULONG ulChildIndex,
				ULONG ulPartIndexId,
				BOOL fOuterPartConsumer,
				CPartIndexMap *ppimResult,
				CPartFilterMap *ppfmResult,
				CColRefSet *pcrsAllowedRefs
				);

			// helper to find join predicates on part keys. Returns NULL if not found
			static
			CExpression *PexprJoinPredOnPartKeys
				(
				IMemoryPool *pmp,
				CExpression *pexprScalar,
				CPartIndexMap *ppimSource,
				ULONG ulPartIndexId,
				CColRefSet *pcrsAllowedRefs
				);

			// are the given predicate parts hash-join compatible?
			static
			BOOL FHashJoinCompatible(CExpression *pexprOuter, CExpression* pexprInner, CExpression *pexprPredOuter, CExpression *pexprPredInner);

		public:

			// match function
			BOOL FMatch(COperator *pop) const;

			// sensitivity to order of inputs
			BOOL FInputOrderSensitive() const
			{
				return true;
			}

			//-------------------------------------------------------------------------------------
			// Required Plan Properties
			//-------------------------------------------------------------------------------------

			// compute required output columns of the n-th child
			virtual
			CColRefSet *PcrsRequired
				(
				IMemoryPool *pmp,
				CExpressionHandle &exprhdl,
				CColRefSet *pcrsRequired,
				ULONG ulChildIndex,
				DrgPdp *pdrgpdpCtxt,
				ULONG ulOptReq
				);

			// compute required ctes of the n-th child
			virtual
			CCTEReq *PcteRequired
				(
				IMemoryPool *pmp,
				CExpressionHandle &exprhdl,
				CCTEReq *pcter,
				ULONG ulChildIndex,
				DrgPdp *pdrgpdpCtxt,
				ULONG ulOptReq
				)
				const;

			// compute required distribution of the n-th child
			virtual
			CDistributionSpec *PdsRequired
				(
				IMemoryPool *pmp,
				CExpressionHandle &exprhdl,
				CDistributionSpec *pdsRequired,
				ULONG ulChildIndex,
				DrgPdp *pdrgpdpCtxt,
				ULONG ulOptReq
				)
				const;
			
			// compute required partition propagation of the n-th child
			virtual
			CPartitionPropagationSpec *PppsRequired
				(
				IMemoryPool *pmp,
				CExpressionHandle &exprhdl,
				CPartitionPropagationSpec *pppsRequired,
				ULONG ulChildIndex,
				DrgPdp *pdrgpdpCtxt,
				ULONG ulOptReq
				);

			// check if required columns are included in output columns
			virtual
			BOOL FProvidesReqdCols(CExpressionHandle &exprhdl, CColRefSet *pcrsRequired, ULONG ulOptReq) const;

			// distribution matching type
			virtual
			CEnfdDistribution::EDistributionMatching Edm
				(
				CReqdPropPlan *prppInput,
				ULONG ulChildIndex,
				DrgPdp *pdrgpdpCtxt,
				ULONG ulOptReq
				);
			
			//-------------------------------------------------------------------------------------
			// Derived Plan Properties
			//-------------------------------------------------------------------------------------

			// derive sort order from outer child
			virtual
			COrderSpec *PosDerive
				(
				IMemoryPool *, // pmp
				CExpressionHandle &exprhdl
				)
				const
			{
				return PosDerivePassThruOuter(exprhdl);
			}

			// derive distribution
			virtual
			CDistributionSpec *PdsDerive(IMemoryPool *pmp, CExpressionHandle &exprhdl) const;

			// derive rewindability
			virtual
			CRewindabilitySpec *PrsDerive(IMemoryPool *pmp, CExpressionHandle &exprhdl) const;

			// derive partition index map
			virtual
			CPartIndexMap *PpimDerive
				(
				IMemoryPool *pmp,
				CExpressionHandle &exprhdl,
				CDrvdPropCtxt * //pdpctxt
				)
				const
			{
				return PpimDeriveCombineRelational(pmp, exprhdl);
			}
			
			// derive partition filter map
			virtual
			CPartFilterMap *PpfmDerive
				(
				IMemoryPool *pmp,
				CExpressionHandle &exprhdl
				)
				const
			{
				// combine part filter maps from relational children
				return PpfmDeriveCombineRelational(pmp, exprhdl);
			}


			//-------------------------------------------------------------------------------------
			// Enforced Properties
			//-------------------------------------------------------------------------------------

			// return rewindability property enforcing type for this operator
			virtual
			CEnfdProp::EPropEnforcingType EpetRewindability
				(
				CExpressionHandle &exprhdl,
				const CEnfdRewindability *per
				)
				const;
				
			// return true if operator passes through stats obtained from children,
			// this is used when computing stats during costing
			virtual
			BOOL FPassThruStats() const
			{
				return false;
			}

			//-------------------------------------------------------------------------------------
			//-------------------------------------------------------------------------------------
			//-------------------------------------------------------------------------------------

			// is given predicate hash-join compatible
			static
			BOOL FHashJoinCompatible(CExpression *pexprPred, CExpression *pexprOuter, CExpression* pexprInner);

			// extract expressions that can be used in hash-join from the given predicate
			static
			void ExtractHashJoinExpressions(CExpression *pexprPred, CExpression **ppexprLeft, CExpression **ppexprRight);

			// can expression be implemented with hash join
			static
			BOOL FHashJoinPossible
				(
				IMemoryPool *pmp,
				CExpression *pexpr,
				DrgPexpr *pdrgpexprOuter,
				DrgPexpr *pdrgpexprInner,
				CExpression **ppexprResult // output: join expression to be tarnsformed to hash join
				);

			// return number of distribution requests for correlated join
			static
			ULONG UlDistrRequestsForCorrelatedJoin();

	}; // class CPhysicalJoin

}

#endif // !GPOPT_CPhysicalJoin_H

// EOF
