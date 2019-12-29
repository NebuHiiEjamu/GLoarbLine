/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
// it deals with byte ordering (MSBF or LSBF)

#ifndef _H_CEndianOrder_
#define _H_CEndianOrder_

#if PRAGMA_ONCE
	#pragma once
#endif

HL_Begin_Namespace_BigRedH

class CEndianOrder
{
	public:
								// by default we are using network ordering
							CEndianOrder()
								// throws nothing
								{ SetOrderMSBF(); } 

							~CEndianOrder()
								// throws nothing
								{}

		void		 		SetOrderLSBF()
								// throws nothing
								{ 
									mOrderLSBF = true;
									#if TARGET_RT_LITTLE_ENDIAN
										mReverseOrder = false;	
									#else
										mReverseOrder = true;	
									#endif
								}		

		void 				SetOrderMSBF()
								// throws nothing
								{ 
									mOrderLSBF = false;
									#if TARGET_RT_LITTLE_ENDIAN
										mReverseOrder = true;	
									#else
										mReverseOrder = false;	
									#endif
								}		

		bool 				IsOrderLSBF()
								// throws nothing
								{ return mOrderLSBF; }

		bool 				IsOrderMSBF()
								// throws nothing
								{ return !mOrderLSBF; }

		bool 				IsOrderReversed()
								// throws nothing
								{ return mReverseOrder; }

		void 				ArrangeOrder(UInt8* inBuf, UInt32 inSize)
								// throws nothing
								{
									if (mReverseOrder)
										for(UInt32 k=0, i=inSize-1; k<i;)
										{
											UInt8 tmp  = inBuf[k];
											inBuf[k++] = inBuf[i];
											inBuf[i--] = tmp;	
										}
								}

		void 				ArrangeOrder(UInt8* inBuf, const UInt8* inSrc, UInt32 inSize)
								// throws nothing
								{
									if (mReverseOrder)
										for(UInt32 k=0, i=inSize; k<inSize;)
											inBuf[--i] = inSrc[k++];
								}		
	private:
		bool 				mOrderLSBF;			
		bool 				mReverseOrder;
}; 

HL_End_Namespace_BigRedH
#endif // _H_CEndianOrder_
