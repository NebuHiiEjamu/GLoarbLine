// =====================================================================
//	TClassCreator.h                      (C) Hotline Communications 1999
// =====================================================================
// Template hierarchy to create classes visible through base classes
// Example of usage at the end of this file

#ifndef _H_TClassCreator_
#define _H_TClassCreator_

#if PRAGMA_ONCE
	#pragma once
#endif
			   
HL_Begin_Namespace_BigRedH

// =====================================================================
//	TBaseCreator                         (C) Hotline Communications 1999
// =====================================================================

template <class Base> 
class TBaseCreator
{
	public:
		virtual Base* 		Create() = 0;
								// throw CMemoryException, ???

}; // class TBaseCreator


// =====================================================================
//	TClassCreator                        (C) Hotline Communications 1999
// =====================================================================

template <class Class, class Base> 
class TClassCreator : public TBaseCreator <Base>
{
	public:
		virtual Base* 		Create()
								// throw CMemoryException, ???
								{ return new Class; }
}; // class TClassCreator


/* Example of usage ====================================================

class CDocumentWindow
{
public:
	typedef TBaseCreator<CWindowImp>	ImpCreator;

	static void 				SetCreator( ImpCreator* inImpCreator );
protected:
	static CWindowImp*			GetFromCreator();
private:
	static auto_ptr<ImpCreator> sCreatorOf_Imp;
};

auto_ptr<CDocumentWindow::ImpCreator> CDocumentWindow::sCreatorOf_Imp (nil);

static void CDocumentWindow::SetCreator( ImpCreator* inImpCreator )
{
	sCreatorOf_Imp = inImpCreator;
}

static Base* CDocumentWindow::GetImpFromCreator()
{
	return sCreatorOf_Imp->Create();
}

void main()
{
CDocumentWindow::SetCreator(new TClassCreator<CDocumentWindowImpWin,CWindowImp>);
}

CDocumentWindow::CDocumentWindow()
: CWindow(CDocumentWindow::GetImpFromCreator())
{}

==================================================== Example of usage */


HL_End_Namespace_BigRedH
#endif // _H_TClassCreator_
