/* $Id: CoinModel.cpp 1509 2011-12-05 13:50:48Z forrest $ */
// Copyright (C) 2005, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinUtilsConfig.h"
#include "CoinHelperFunctions.hpp"
#include "CoinModel.hpp"
#include "CoinSort.hpp"
#include "CoinMpsIO.hpp"
#include "CoinFloatEqual.hpp"

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
CoinBaseModel::CoinBaseModel () 
  :  numberRows_(0),
     numberColumns_(0),
     optimizationDirection_(1.0),
     objectiveOffset_(0.0),
     logLevel_(0)
{
  problemName_ = "";
  rowBlockName_ = "row_master";
  columnBlockName_ = "column_master";
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
CoinBaseModel::CoinBaseModel (const CoinBaseModel & rhs) 
  : numberRows_(rhs.numberRows_),
    numberColumns_(rhs.numberColumns_),
    optimizationDirection_(rhs.optimizationDirection_),
    objectiveOffset_(rhs.objectiveOffset_),
    logLevel_(rhs.logLevel_)
{
  problemName_ = rhs.problemName_;
  rowBlockName_ = rhs.rowBlockName_;
  columnBlockName_ = rhs.columnBlockName_;
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
CoinBaseModel::~CoinBaseModel ()
{
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
CoinBaseModel &
CoinBaseModel::operator=(const CoinBaseModel& rhs)
{
  if (this != &rhs) {
    problemName_ = rhs.problemName_;
    rowBlockName_ = rhs.rowBlockName_;
    columnBlockName_ = rhs.columnBlockName_;
    numberRows_ = rhs.numberRows_;
    numberColumns_ = rhs.numberColumns_;
    optimizationDirection_ = rhs.optimizationDirection_;
    objectiveOffset_ = rhs.objectiveOffset_;
    logLevel_ = rhs.logLevel_;
  }
  return *this;
}
void 
CoinBaseModel::setLogLevel(int value)
{
  if (value>=0&&value<3)
    logLevel_=value;
}
void
CoinBaseModel::setProblemName (const char *name)
{ 
  if (name)
    problemName_ = name ;
  else
    problemName_ = "";
}
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
CoinModel::CoinModel () 
  :  CoinBaseModel(),
     maximumRows_(0),
     maximumColumns_(0),
     numberElements_(0),
     maximumElements_(0),
     numberQuadraticElements_(0),
     maximumQuadraticElements_(0),
     rowLower_(NULL),
     rowUpper_(NULL),
     rowType_(NULL),
     objective_(NULL),
     columnLower_(NULL),
     columnUpper_(NULL),
     integerType_(NULL),
     columnType_(NULL),
     start_(NULL),
     elements_(NULL),
     packedMatrix_(NULL),
     quadraticElements_(NULL),
     sortIndices_(NULL),
     sortElements_(NULL),
     sortSize_(0),
     sizeAssociated_(0),
     associated_(NULL),
     numberSOS_(0),
     startSOS_(NULL),
     memberSOS_(NULL),
     typeSOS_(NULL),
     prioritySOS_(NULL),
     referenceSOS_(NULL),
     priority_(NULL),
     cut_(NULL),
     moreInfo_(NULL),
     type_(-1),
     noNames_(false),
     links_(0)
{
}
/* Constructor with sizes. */
CoinModel::CoinModel(int firstRows, int firstColumns, int firstElements,bool noNames)
  :  CoinBaseModel(),
     maximumRows_(0),
     maximumColumns_(0),
     numberElements_(0),
     maximumElements_(0),
     numberQuadraticElements_(0),
     maximumQuadraticElements_(0),
     rowLower_(NULL),
     rowUpper_(NULL),
     rowType_(NULL),
     objective_(NULL),
     columnLower_(NULL),
     columnUpper_(NULL),
     integerType_(NULL),
     columnType_(NULL),
     start_(NULL),
     elements_(NULL),
     packedMatrix_(NULL),
     quadraticElements_(NULL),
     sortIndices_(NULL),
     sortElements_(NULL),
     sortSize_(0),
     sizeAssociated_(0),
     associated_(NULL),
     numberSOS_(0),
     startSOS_(NULL),
     memberSOS_(NULL),
     typeSOS_(NULL),
     prioritySOS_(NULL),
     referenceSOS_(NULL),
     priority_(NULL),
     cut_(NULL),
     moreInfo_(NULL),
     type_(-1),
     noNames_(noNames),
     links_(0)
{
  if (!firstRows) {
    if (firstColumns) {
      type_=1;
      resize(0,firstColumns,firstElements);
    }
  } else {
    type_=0;
    resize(firstRows,0,firstElements);
    if (firstColumns) {
      // mixed - do linked lists for columns
      //createList(2);
    }
  }
}
/* Read a problem in MPS or GAMS format from the given filename.
 */
CoinModel::CoinModel(const char *fileName, int allowStrings)
  : CoinBaseModel(),
    maximumRows_(0),
    maximumColumns_(0),
    numberElements_(0),
    maximumElements_(0),
    numberQuadraticElements_(0),
    maximumQuadraticElements_(0),
    rowLower_(NULL),
    rowUpper_(NULL),
    rowType_(NULL),
    objective_(NULL),
    columnLower_(NULL),
    columnUpper_(NULL),
    integerType_(NULL),
    columnType_(NULL),
    start_(NULL),
    elements_(NULL),
    packedMatrix_(NULL),
    quadraticElements_(NULL),
    sortIndices_(NULL),
    sortElements_(NULL),
    sortSize_(0),
    sizeAssociated_(0),
    associated_(NULL),
    numberSOS_(0),
    startSOS_(NULL),
    memberSOS_(NULL),
    typeSOS_(NULL),
    prioritySOS_(NULL),
    referenceSOS_(NULL),
    priority_(NULL),
    cut_(NULL),
    moreInfo_(NULL),
    type_(-1),
    noNames_(false),
    links_(0)
{
  rowBlockName_ = "row_master";
  columnBlockName_ = "column_master";
  int status=0;
  if (!strcmp(fileName,"-")||!strcmp(fileName,"stdin")) {
    // stdin
  } else {
    std::string name=fileName;
    bool readable = fileCoinReadable(name);
    if (!readable) {
      std::cerr<<"Unable to open file "
	       <<fileName<<std::endl;
      status = -1;
    }
  }
  CoinMpsIO m;
  m.setAllowStringElements(allowStrings);
  m.setConvertObjective(true);
  if (!status) {
    try {
      status=m.readMps(fileName,"");
    }
    catch (CoinError& e) {
      e.print();
      status=-1;
    }
  }
  if (!status) {
    // set problem name
    problemName_=m.getProblemName();
    objectiveOffset_ = m.objectiveOffset();
    // build model
    int numberRows = m.getNumRows();
    int numberColumns = m.getNumCols();
    
    // Build by row from scratch
    CoinPackedMatrix matrixByRow = * m.getMatrixByRow();
    const double * element = matrixByRow.getElements();
    const int * column = matrixByRow.getIndices();
    const CoinBigIndex * rowStart = matrixByRow.getVectorStarts();
    const int * rowLength = matrixByRow.getVectorLengths();
    const double * rowLower = m.getRowLower();
    const double * rowUpper = m.getRowUpper();
    const double * columnLower = m.getColLower();
    const double * columnUpper = m.getColUpper();
    const double * objective = m.getObjCoefficients();
    int i;
    for (i=0;i<numberRows;i++) {
      addRow(rowLength[i],column+rowStart[i],
	     element+rowStart[i],rowLower[i],rowUpper[i],m.rowName(i));
    }
    int numberIntegers = 0;
    // Now do column part
    for (i=0;i<numberColumns;i++) {
      setColumnBounds(i,columnLower[i],columnUpper[i]);
      setColumnObjective(i,objective[i]);
      if (m.isInteger(i)) {
	setColumnIsInteger(i,true);;
	numberIntegers++;
      }
    }
    bool quadraticInteger = (numberIntegers!=0)&&m.reader()->whichSection (  ) == COIN_QUAD_SECTION ;
    // do names
    int iRow;
    for (iRow=0;iRow<numberRows_;iRow++) {
      const char * name = m.rowName(iRow);
      setRowName(iRow,name);
    }
    bool ifStrings = ( m.numberStringElements() != 0 );
    int nChanged=0;
    int iColumn;
    for (iColumn=0;iColumn<numberColumns_;iColumn++) {
      // Replace - + or * if strings
      if (!ifStrings&&!quadraticInteger) {
	const char * name = m.columnName(iColumn);
	setColumnName(iColumn,name);
      } else {
	assert (strlen(m.columnName(iColumn))<100);
	char temp[100];
	strcpy(temp,m.columnName(iColumn));
	int n=CoinStrlenAsInt(temp);
	bool changed=false;
	for (int i=0;i<n;i++) {
	  if (temp[i]=='-') {
	    temp[i]='_';
	    changed=true;
	  } else
	  if (temp[i]=='+') {
	    temp[i]='$';
	    changed=true;
	  } else
	  if (temp[i]=='*') {
	    temp[i]='&';
	    changed=true;
	  }
	}
	if (changed)
	  nChanged++;
	setColumnName(iColumn,temp);
      }
    }
    if (nChanged) 
      printf("%d column names changed to eliminate - + or *\n",nChanged);
    if (ifStrings) {
      // add in 
      int numberElements = m.numberStringElements();
      for (int i=0;i<numberElements;i++) {
	const char * line = m.stringElement(i);
	int iRow;
	int iColumn;
	sscanf(line,"%d,%d,",&iRow,&iColumn);
	assert (iRow>=0&&iRow<=numberRows_+2);
	assert (iColumn>=0&&iColumn<=numberColumns_);
	const char * pos = strchr(line,',');
	assert (pos);
	pos = strchr(pos+1,',');
	assert (pos);
	pos++;
	if (iRow<numberRows_&&iColumn<numberColumns_) {
	  // element
	  setElement(iRow,iColumn,pos);
	} else {
	  fprintf(stderr,"code CoinModel strings for rim\n");
	  abort();
	}
      }
    }
    // get quadratic part
    if (m.reader()->whichSection (  ) == COIN_QUAD_SECTION ) {
      int * start=NULL;
      int * column = NULL;
      double * element = NULL;
      status=m.readQuadraticMps(NULL,start,column,element,2);
      if (!status) {
	// If strings allowed 13 then just for Hans convert to constraint
	int objRow=-1;
	if (allowStrings==13) {
	  int objColumn=numberColumns_;
	  objRow=numberRows_;
	  // leave linear part in objective
	  addColumn(0,NULL,NULL,-COIN_DBL_MAX,COIN_DBL_MAX,1.0,"obj");
	  double minusOne=-1.0;
	  addRow(1,&objColumn,&minusOne,-COIN_DBL_MAX,0.0,"objrow");
	}
	if (!ifStrings&&!numberIntegers) {
	  // no strings - add to quadratic (not done yet)
	  for (int iColumn=0;iColumn<numberColumns_;iColumn++) {
	    for (CoinBigIndex j = start[iColumn];j<start[iColumn+1];j++) {
	      int jColumn = column[j];
	      double value = element[j];
	      // what about diagonal etc
	      if (jColumn==iColumn) {
		printf("diag %d %d %g\n",iColumn,jColumn,value);
		setQuadraticElement(iColumn,jColumn,0.5*value);
	      } else if (jColumn>iColumn) {
		printf("above diag %d %d %g\n",iColumn,jColumn,value);
	      } else if (jColumn<iColumn) {
		printf("below diag %d %d %g\n",iColumn,jColumn,value);
		setQuadraticElement(iColumn,jColumn,value);
	      }
	    }
	  }
	} else {
	  // add in as strings
	  for (int iColumn=0;iColumn<numberColumns_;iColumn++) {
	    char temp[20000];
	    temp[0]='\0';
	    int put=0;
	    int n=0;
	    bool ifFirst=true;
	    double value = getColumnObjective(iColumn);
	    if (value&&objRow<0) {
	      sprintf(temp,"%g",value);
	      ifFirst=false;
              /* static cast is safe, temp is at most 20000 chars */
	      put = CoinStrlenAsInt(temp);
	    }
	    for (CoinBigIndex j = start[iColumn];j<start[iColumn+1];j++) {
	      int jColumn = column[j];
	      double value = element[j];
	      // what about diagonal etc
	      if (jColumn==iColumn) {
		//printf("diag %d %d %g\n",iColumn,jColumn,value);
		value *= 0.5;
	      } else if (jColumn>iColumn) {
		//printf("above diag %d %d %g\n",iColumn,jColumn,value);
	      } else if (jColumn<iColumn) {
		//printf("below diag %d %d %g\n",iColumn,jColumn,value);
		value=0.0;
	      }
	      if (value) {
		n++;
		const char * name = columnName(jColumn);
		if (value==1.0) {
		  sprintf(temp+put,"%s%s",ifFirst ? "" : "+",name);
		} else {
		  if (ifFirst||value<0.0)
		    sprintf(temp+put,"%g*%s",value,name);
		  else
		    sprintf(temp+put,"+%g*%s",value,name);
		}
		put += CoinStrlenAsInt(temp+put);
		assert (put<20000);
		ifFirst=false;
	      }
	    }
	    if (n) {
	      if (objRow<0)
		setObjective(iColumn,temp);
	      else
		setElement(objRow,iColumn,temp);
	      //printf("el for objective column c%7.7d is %s\n",iColumn,temp);
	    }
	  }
	}
      } 
      delete [] start;
      delete [] column;
      delete [] element;
    }
  }
}
// From arrays
CoinModel::CoinModel(int numberRows, int numberColumns,
	    const CoinPackedMatrix * matrix,
	    const double * rowLower, const double * rowUpper,
	    const double * columnLower, const double * columnUpper,
	    const double * objective)
  :  CoinBaseModel(),
     maximumRows_(numberRows),
     maximumColumns_(numberColumns),
     numberElements_(matrix->getNumElements()),
     maximumElements_(matrix->getNumElements()),
     numberQuadraticElements_(0),
     maximumQuadraticElements_(0),
     rowType_(NULL),
     integerType_(NULL),
     columnType_(NULL),
     start_(NULL),
     elements_(NULL),
     packedMatrix_(NULL),
     quadraticElements_(NULL),
     sortIndices_(NULL),
     sortElements_(NULL),
     sortSize_(0),
     sizeAssociated_(0),
     associated_(NULL),
     numberSOS_(0),
     startSOS_(NULL),
     memberSOS_(NULL),
     typeSOS_(NULL),
     prioritySOS_(NULL),
     referenceSOS_(NULL),
     priority_(NULL),
     cut_(NULL),
     moreInfo_(NULL),
     type_(-1),
     noNames_(false),
     links_(0)
{
  numberRows_ = numberRows;
  numberColumns_ = numberColumns;
  assert (numberRows_>=matrix->getNumRows());
  assert (numberColumns_>=matrix->getNumCols());
  type_ = 3;
  packedMatrix_ = new CoinPackedMatrix(*matrix);
  rowLower_ = CoinCopyOfArray(rowLower,numberRows_);
  rowUpper_ = CoinCopyOfArray(rowUpper,numberRows_);
  objective_ = CoinCopyOfArray(objective,numberColumns_);
  columnLower_ = CoinCopyOfArray(columnLower,numberColumns_);
  columnUpper_ = CoinCopyOfArray(columnUpper,numberColumns_);
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
CoinModel::CoinModel (const CoinModel & rhs) 
  : CoinBaseModel(rhs),
    maximumRows_(rhs.maximumRows_),
    maximumColumns_(rhs.maximumColumns_),
    numberElements_(rhs.numberElements_),
    maximumElements_(rhs.maximumElements_),
    numberQuadraticElements_(rhs.numberQuadraticElements_),
    maximumQuadraticElements_(rhs.maximumQuadraticElements_),
    rowName_(rhs.rowName_),
    columnName_(rhs.columnName_),
    string_(rhs.string_),
    hashElements_(rhs.hashElements_),
    rowList_(rhs.rowList_),
    columnList_(rhs.columnList_),
    hashQuadraticElements_(rhs.hashQuadraticElements_),
    sortSize_(rhs.sortSize_),
    quadraticRowList_(rhs.quadraticRowList_),
    quadraticColumnList_(rhs.quadraticColumnList_),
    sizeAssociated_(rhs.sizeAssociated_),
    numberSOS_(rhs.numberSOS_),
    type_(rhs.type_),
    noNames_(rhs.noNames_),
    links_(rhs.links_)
{
  rowLower_ = CoinCopyOfArray(rhs.rowLower_,maximumRows_);
  rowUpper_ = CoinCopyOfArray(rhs.rowUpper_,maximumRows_);
  rowType_ = CoinCopyOfArray(rhs.rowType_,maximumRows_);
  objective_ = CoinCopyOfArray(rhs.objective_,maximumColumns_);
  columnLower_ = CoinCopyOfArray(rhs.columnLower_,maximumColumns_);
  columnUpper_ = CoinCopyOfArray(rhs.columnUpper_,maximumColumns_);
  integerType_ = CoinCopyOfArray(rhs.integerType_,maximumColumns_);
  columnType_ = CoinCopyOfArray(rhs.columnType_,maximumColumns_);
  sortIndices_ = CoinCopyOfArray(rhs.sortIndices_,sortSize_);
  sortElements_ = CoinCopyOfArray(rhs.sortElements_,sortSize_);
  associated_ = CoinCopyOfArray(rhs.associated_,sizeAssociated_);
  priority_ = CoinCopyOfArray(rhs.priority_,maximumColumns_);
  cut_ = CoinCopyOfArray(rhs.cut_,maximumRows_);
  moreInfo_ = rhs.moreInfo_;
  if (rhs.packedMatrix_)
    packedMatrix_ = new CoinPackedMatrix(*rhs.packedMatrix_);
  else
    packedMatrix_ = NULL;
  if (numberSOS_) {
    startSOS_ = CoinCopyOfArray(rhs.startSOS_,numberSOS_+1);
    int numberMembers = startSOS_[numberSOS_];
    memberSOS_ = CoinCopyOfArray(rhs.memberSOS_,numberMembers);
    typeSOS_ = CoinCopyOfArray(rhs.typeSOS_,numberSOS_);
    prioritySOS_ = CoinCopyOfArray(rhs.prioritySOS_,numberSOS_);
    referenceSOS_ = CoinCopyOfArray(rhs.referenceSOS_,numberMembers);
  } else {
    startSOS_ = NULL;
    memberSOS_ = NULL;
    typeSOS_ = NULL;
    prioritySOS_ = NULL;
    referenceSOS_ = NULL;
  }
  if (type_==0) {
    start_ = CoinCopyOfArray(rhs.start_,maximumRows_+1);
  } else if (type_==1) {
    start_ = CoinCopyOfArray(rhs.start_,maximumColumns_+1);
  } else {
    start_=NULL;
  }
  elements_ = CoinCopyOfArray(rhs.elements_,maximumElements_);
  quadraticElements_ = CoinCopyOfArray(rhs.quadraticElements_,maximumQuadraticElements_);
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
CoinModel::~CoinModel ()
{
  delete [] rowLower_;
  delete [] rowUpper_;
  delete [] rowType_;
  delete [] objective_;
  delete [] columnLower_;
  delete [] columnUpper_;
  delete [] integerType_;
  delete [] columnType_;
  delete [] start_;
  delete [] elements_;
  delete [] quadraticElements_;
  delete [] sortIndices_;
  delete [] sortElements_;
  delete [] associated_;
  delete [] startSOS_;
  delete [] memberSOS_;
  delete [] typeSOS_;
  delete [] prioritySOS_;
  delete [] referenceSOS_;
  delete [] priority_;
  delete [] cut_;
  delete packedMatrix_;
}
// Clone
CoinBaseModel *
CoinModel::clone() const
{
  return new CoinModel(*this);
}


//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
CoinModel &
CoinModel::operator=(const CoinModel& rhs)
{
  if (this != &rhs) {
    CoinBaseModel::operator=(rhs);
    delete [] rowLower_;
    delete [] rowUpper_;
    delete [] rowType_;
    delete [] objective_;
    delete [] columnLower_;
    delete [] columnUpper_;
    delete [] integerType_;
    delete [] columnType_;
    delete [] start_;
    delete [] elements_;
    delete [] quadraticElements_;
    delete [] sortIndices_;
    delete [] sortElements_;
    delete [] associated_;
    delete [] startSOS_;
    delete [] memberSOS_;
    delete [] typeSOS_;
    delete [] prioritySOS_;
    delete [] referenceSOS_;
    delete [] priority_;
    delete [] cut_;
    delete packedMatrix_;
    maximumRows_ = rhs.maximumRows_;
    maximumColumns_ = rhs.maximumColumns_;
    numberElements_ = rhs.numberElements_;
    maximumElements_ = rhs.maximumElements_;
    numberQuadraticElements_ = rhs.numberQuadraticElements_;
    maximumQuadraticElements_ = rhs.maximumQuadraticElements_;
    sortSize_ = rhs.sortSize_;
    rowName_ = rhs.rowName_;
    columnName_ = rhs.columnName_;
    string_ = rhs.string_;
    hashElements_=rhs.hashElements_;
    hashQuadraticElements_=rhs.hashQuadraticElements_;
    rowList_ = rhs.rowList_;
    quadraticColumnList_ = rhs.quadraticColumnList_;
    quadraticRowList_ = rhs.quadraticRowList_;
    columnList_ = rhs.columnList_;
    sizeAssociated_= rhs.sizeAssociated_;
    numberSOS_ = rhs.numberSOS_;
    type_ = rhs.type_;
    noNames_ = rhs.noNames_;
    links_ = rhs.links_;
    rowLower_ = CoinCopyOfArray(rhs.rowLower_,maximumRows_);
    rowUpper_ = CoinCopyOfArray(rhs.rowUpper_,maximumRows_);
    rowType_ = CoinCopyOfArray(rhs.rowType_,maximumRows_);
    objective_ = CoinCopyOfArray(rhs.objective_,maximumColumns_);
    columnLower_ = CoinCopyOfArray(rhs.columnLower_,maximumColumns_);
    columnUpper_ = CoinCopyOfArray(rhs.columnUpper_,maximumColumns_);
    integerType_ = CoinCopyOfArray(rhs.integerType_,maximumColumns_);
    columnType_ = CoinCopyOfArray(rhs.columnType_,maximumColumns_);
    priority_ = CoinCopyOfArray(rhs.priority_,maximumColumns_);
    cut_ = CoinCopyOfArray(rhs.cut_,maximumRows_);
    moreInfo_ = rhs.moreInfo_;
    if (rhs.packedMatrix_)
      packedMatrix_ = new CoinPackedMatrix(*rhs.packedMatrix_);
    else
      packedMatrix_ = NULL;
    if (numberSOS_) {
      startSOS_ = CoinCopyOfArray(rhs.startSOS_,numberSOS_+1);
      int numberMembers = startSOS_[numberSOS_];
      memberSOS_ = CoinCopyOfArray(rhs.memberSOS_,numberMembers);
      typeSOS_ = CoinCopyOfArray(rhs.typeSOS_,numberSOS_);
      prioritySOS_ = CoinCopyOfArray(rhs.prioritySOS_,numberSOS_);
      referenceSOS_ = CoinCopyOfArray(rhs.referenceSOS_,numberMembers);
    } else {
      startSOS_ = NULL;
      memberSOS_ = NULL;
      typeSOS_ = NULL;
      prioritySOS_ = NULL;
      referenceSOS_ = NULL;
    }
    if (type_==0) {
      start_ = CoinCopyOfArray(rhs.start_,maximumRows_+1);
    } else if (type_==1) {
      start_ = CoinCopyOfArray(rhs.start_,maximumColumns_+1);
    } else {
      start_=NULL;
    }
    elements_ = CoinCopyOfArray(rhs.elements_,maximumElements_);
    quadraticElements_ = CoinCopyOfArray(rhs.quadraticElements_,maximumQuadraticElements_);
    sortIndices_ = CoinCopyOfArray(rhs.sortIndices_,sortSize_);
    sortElements_ = CoinCopyOfArray(rhs.sortElements_,sortSize_);
    associated_ = CoinCopyOfArray(rhs.associated_,sizeAssociated_);
  }
  return *this;
}
/* add a row -  numberInRow may be zero */
void 
CoinModel::addRow(int numberInRow, const int * columns,
                  const double * elements, double rowLower, 
                  double rowUpper, const char * name)
{
  if (type_==-1) {
    // initial
    type_=0;
    resize(100,0,1000);
  } else if (type_==1) {
    // mixed - do linked lists for rows
    createList(1);
  } else if (type_==3) {
    badType();
  }
  int newColumn=-1;
  if (numberInRow>0) {
    // Move and sort
    if (numberInRow>sortSize_) {
      delete [] sortIndices_;
      delete [] sortElements_;
      sortSize_ = numberInRow+100; 
      sortIndices_ = new int [sortSize_];
      sortElements_ = new double [sortSize_];
    }
    bool sorted = true;
    int last=-1;
    int i;
    for (i=0;i<numberInRow;i++) {
      int k=columns[i];
      if (k<=last)
        sorted=false;
      last=k;
      sortIndices_[i]=k;
      sortElements_[i]=elements[i];
    }
    if (!sorted) {
      CoinSort_2(sortIndices_,sortIndices_+numberInRow,sortElements_);
    }
    // check for duplicates etc
    if (sortIndices_[0]<0) {
      printf("bad index %d\n",sortIndices_[0]);
      // clean up
      abort();
    }
    last=-1;
    bool duplicate=false;
    for (i=0;i<numberInRow;i++) {
      int k=sortIndices_[i];
      if (k==last)
        duplicate=true;
      last=k;
    }
    if (duplicate) {
      printf("duplicates - what do we want\n");
      abort();
    }
    newColumn = CoinMax(newColumn,last);
  }
  int newRow=0;
  int newElement=0;
  if (numberElements_+numberInRow>maximumElements_) {
    newElement = (3*(numberElements_+numberInRow)/2) + 1000;
    if (numberRows_*10>maximumRows_*9)
      newRow = (maximumRows_*3)/2+100;
  }
  if (numberRows_==maximumRows_)
    newRow = (maximumRows_*3)/2+100;
  if (newRow||newColumn>=maximumColumns_||newElement) {
    if (newColumn<maximumColumns_) {
      // columns okay
      resize(newRow,0,newElement);
    } else {
      // newColumn will be new numberColumns_
      resize(newRow,(3*newColumn)/2+100,newElement);
    }
  }
  // If rows extended - take care of that
  fillRows(numberRows_,false,true);
  // Do name
  if (name) {
    rowName_.addHash(numberRows_,name);
  } else if (!noNames_) {
    char name[9];
    sprintf(name,"r%7.7d",numberRows_);
    rowName_.addHash(numberRows_,name);
  }
  rowLower_[numberRows_]=rowLower;
  rowUpper_[numberRows_]=rowUpper;
  // If columns extended - take care of that
  fillColumns(newColumn,false);
  if (type_==0) {
    // can do simply
    int put = start_[numberRows_];
    assert (put==numberElements_);
    bool doHash = hashElements_.numberItems()!=0;
    for (int i=0;i<numberInRow;i++) {
      setRowAndStringInTriple(elements_[put],numberRows_,false);
      //elements_[put].row=static_cast<unsigned int>(numberRows_);
      //elements_[put].string=0;
      elements_[put].column=sortIndices_[i];
      elements_[put].value=sortElements_[i];
      if (doHash)
        hashElements_.addHash(put,numberRows_,sortIndices_[i],elements_);
      put++;
    }
    start_[numberRows_+1]=put;
    numberElements_+=numberInRow;
  } else {
    if (numberInRow) {
      // must update at least one link
      assert (links_);
      if (links_==1||links_==3) {
        int first = rowList_.addEasy(numberRows_,numberInRow,sortIndices_,sortElements_,elements_,
                                     hashElements_);
        if (links_==3)
          columnList_.addHard(first,elements_,rowList_.firstFree(),rowList_.lastFree(),
                              rowList_.next());
        numberElements_=CoinMax(numberElements_,rowList_.numberElements());
        if (links_==3)
          assert (columnList_.numberElements()==rowList_.numberElements());
      } else if (links_==2) {
        columnList_.addHard(numberRows_,numberInRow,sortIndices_,sortElements_,elements_,
                            hashElements_);
        numberElements_=CoinMax(numberElements_,columnList_.numberElements());
      }
    }
    numberElements_=CoinMax(numberElements_,hashElements_.numberItems());
  }
  numberRows_++;
}
// add a column - numberInColumn may be zero */
void 
CoinModel::addColumn(int numberInColumn, const int * rows,
                     const double * elements, 
                     double columnLower, 
                     double columnUpper, double objectiveValue,
                     const char * name, bool isInteger)
{
  if (type_==-1) {
    // initial
    type_=1;
    resize(0,100,1000);
  } else if (type_==0) {
    // mixed - do linked lists for columns
    createList(2);
  } else if (type_==3) {
    badType();
  }
  int newRow=-1;
  if (numberInColumn>0) {
    // Move and sort
    if (numberInColumn>sortSize_) {
      delete [] sortIndices_;
      delete [] sortElements_;
      sortSize_ = numberInColumn+100; 
      sortIndices_ = new int [sortSize_];
      sortElements_ = new double [sortSize_];
    }
    bool sorted = true;
    int last=-1;
    int i;
    for (i=0;i<numberInColumn;i++) {
      int k=rows[i];
      if (k<=last)
        sorted=false;
      last=k;
      sortIndices_[i]=k;
      sortElements_[i]=elements[i];
    }
    if (!sorted) {
      CoinSort_2(sortIndices_,sortIndices_+numberInColumn,sortElements_);
    }
    // check for duplicates etc
    if (sortIndices_[0]<0) {
      printf("bad index %d\n",sortIndices_[0]);
      // clean up
      abort();
    }
    last=-1;
    bool duplicate=false;
    for (i=0;i<numberInColumn;i++) {
      int k=sortIndices_[i];
      if (k==last)
        duplicate=true;
      last=k;
    }
    if (duplicate) {
      printf("duplicates - what do we want\n");
      abort();
    }
    newRow = CoinMax(newRow,last);
  }
  int newColumn=0;
  int newElement=0;
  if (numberElements_+numberInColumn>maximumElements_) {
    newElement = (3*(numberElements_+numberInColumn)/2) + 1000;
    if (numberColumns_*10>maximumColumns_*9)
      newColumn = (maximumColumns_*3)/2+100;
  }
  if (numberColumns_==maximumColumns_)
    newColumn = (maximumColumns_*3)/2+100;
  if (newColumn||newRow>=maximumRows_||newElement) {
    if (newRow<maximumRows_) {
      // rows okay
      resize(0,newColumn,newElement);
    } else {
      // newRow will be new numberRows_
      resize((3*newRow)/2+100,newColumn,newElement);
    }
  }
  // If columns extended - take care of that
  fillColumns(numberColumns_,false,true);
  // Do name
  if (name) {
    columnName_.addHash(numberColumns_,name);
  } else if (!noNames_) {
    char name[9];
    sprintf(name,"c%7.7d",numberColumns_);
    columnName_.addHash(numberColumns_,name);
  }
  columnLower_[numberColumns_]=columnLower;
  columnUpper_[numberColumns_]=columnUpper;
  objective_[numberColumns_]=objectiveValue;
  if (isInteger)
    integerType_[numberColumns_]=1;
  else
    integerType_[numberColumns_]=0;
  // If rows extended - take care of that
  fillRows(newRow,false);
  if (type_==1) {
    // can do simply
    int put = start_[numberColumns_];
    assert (put==numberElements_);
    bool doHash = hashElements_.numberItems()!=0;
    for (int i=0;i<numberInColumn;i++) {
      elements_[put].column=numberColumns_;
      setRowAndStringInTriple(elements_[put],sortIndices_[i],false);
      //elements_[put].string=0;
      //elements_[put].row=static_cast<unsigned int>(sortIndices_[i]);
      elements_[put].value=sortElements_[i];
      if (doHash)
        hashElements_.addHash(put,sortIndices_[i],numberColumns_,elements_);
      put++;
    }
    start_[numberColumns_+1]=put;
    numberElements_+=numberInColumn;
  } else {
    if (numberInColumn) {
      // must update at least one link
      assert (links_);
      if (links_==2||links_==3) {
        int first = columnList_.addEasy(numberColumns_,numberInColumn,sortIndices_,sortElements_,elements_,
                                        hashElements_);
        if (links_==3)
          rowList_.addHard(first,elements_,columnList_.firstFree(),columnList_.lastFree(),
                              columnList_.next());
        numberElements_=CoinMax(numberElements_,columnList_.numberElements());
        if (links_==3)
          assert (columnList_.numberElements()==rowList_.numberElements());
      } else if (links_==1) {
        rowList_.addHard(numberColumns_,numberInColumn,sortIndices_,sortElements_,elements_,
                         hashElements_);
        numberElements_=CoinMax(numberElements_,rowList_.numberElements());
      }
    }
  }
  numberColumns_++;
}
// Sets value for row i and column j 
void 
CoinModel::setElement(int i,int j,double value) 
{
  if (type_==-1) {
    // initial
    type_=0;
    resize(100,100,1000);
    createList(2);
  } else if (type_==3) {
    badType();
  } else if (!links_) {
    if (type_==0||type_==2) {
      createList(1);
    } else if(type_==1) {
      createList(2);
    }
  }
  if (!hashElements_.maximumItems()) {
    hashElements_.resize(maximumElements_,elements_);
  }
  int position = hashElements_.hash(i,j,elements_);
  if (position>=0) {
    elements_[position].value=value;
    setStringInTriple(elements_[position],false);
  } else {
    int newColumn=0;
    if (j>=maximumColumns_) {
      newColumn = j+1;
    }
    int newRow=0;
    if (i>=maximumRows_) {
      newRow = i+1;
    }
    int newElement=0;
    if (numberElements_==maximumElements_) {
      newElement = (3*numberElements_/2) + 1000;
    }
    if (newRow||newColumn||newElement) {
      if (newColumn) 
        newColumn = (3*newColumn)/2+100;
      if (newRow) 
        newRow = (3*newRow)/2+100;
      resize(newRow,newColumn,newElement);
    }
    // If columns extended - take care of that
    fillColumns(j,false);
    // If rows extended - take care of that
    fillRows(i,false);
    // treat as addRow unless only columnList_ exists
    if ((links_&1)!=0) {
      int first = rowList_.addEasy(i,1,&j,&value,elements_,hashElements_);
      if (links_==3)
        columnList_.addHard(first,elements_,rowList_.firstFree(),rowList_.lastFree(),
                            rowList_.next());
      numberElements_=CoinMax(numberElements_,rowList_.numberElements());
      if (links_==3)
        assert (columnList_.numberElements()==rowList_.numberElements());
    } else if (links_==2) {
      columnList_.addHard(i,1,&j,&value,elements_,hashElements_);
      numberElements_=CoinMax(numberElements_,columnList_.numberElements());
    }
    numberRows_=CoinMax(numberRows_,i+1);;
    numberColumns_=CoinMax(numberColumns_,j+1);;
  }
}
// Sets quadratic value for column i and j 
void 
CoinModel::setQuadraticElement(int ,int ,double ) 
{
  printf("not written yet\n");
  abort();
  return;
}
// Sets value for row i and column j as string
void 
CoinModel::setElement(int i,int j,const char * value) 
{
  double dummyValue=1.0;
  if (type_==-1) {
    // initial
    type_=0;
    resize(100,100,1000);
    createList(2);
  } else if (type_==3) {
    badType();
  } else if (!links_) {
    if (type_==0||type_==2) {
      createList(1);
    } else if(type_==1) {
      createList(2);
    }
  }
  if (!hashElements_.maximumItems()) {
    // set up number of items
    hashElements_.setNumberItems(numberElements_);
    hashElements_.resize(maximumElements_,elements_);
  }
  int position = hashElements_.hash(i,j,elements_);
  if (position>=0) {
    int iValue = addString(value);
    elements_[position].value=iValue;
    setStringInTriple(elements_[position],true);
  } else {
    int newColumn=0;
    if (j>=maximumColumns_) {
      newColumn = j+1;
    }
    int newRow=0;
    if (i>=maximumRows_) {
      newRow = i+1;
    }
    int newElement=0;
    if (numberElements_==maximumElements_) {
      newElement = (3*numberElements_/2) + 1000;
    }
    if (newRow||newColumn||newElement) {
      if (newColumn) 
        newColumn = (3*newColumn)/2+100;
      if (newRow) 
        newRow = (3*newRow)/2+100;
      resize(newRow,newColumn,newElement);
    }
    // If columns extended - take care of that
    fillColumns(j,false);
    // If rows extended - take care of that
    fillRows(i,false);
    // treat as addRow unless only columnList_ exists
    if ((links_&1)!=0) {
      int first = rowList_.addEasy(i,1,&j,&dummyValue,elements_,hashElements_);
      if (links_==3)
        columnList_.addHard(first,elements_,rowList_.firstFree(),rowList_.lastFree(),
                            rowList_.next());
      numberElements_=CoinMax(numberElements_,rowList_.numberElements());
      if (links_==3)
        assert (columnList_.numberElements()==rowList_.numberElements());
    } else if (links_==2) {
      columnList_.addHard(i,1,&j,&dummyValue,elements_,hashElements_);
      numberElements_=CoinMax(numberElements_,columnList_.numberElements());
    }
    numberRows_=CoinMax(numberRows_,i+1);;
    numberColumns_=CoinMax(numberColumns_,j+1);;
    int position = hashElements_.hash(i,j,elements_);
    assert (position>=0);
    int iValue = addString(value);
    elements_[position].value=iValue;
    setStringInTriple(elements_[position],true);
  }
}
// Associates a string with a value.  Returns string id (or -1 if does not exist)
int 
CoinModel::associateElement(const char * stringValue, double value)
{
  int position = string_.hash(stringValue);
  if (position<0) {
    // not there -add
    position = addString(stringValue);
    assert (position==string_.numberItems()-1);
  }
  if (sizeAssociated_<=position) {
    int newSize = (3*position)/2+100;
    double * temp = new double[newSize];
    CoinMemcpyN(associated_,sizeAssociated_,temp);
    CoinFillN(temp+sizeAssociated_,newSize-sizeAssociated_,unsetValue());
    delete [] associated_;
    associated_ = temp;
    sizeAssociated_=newSize;
  }
  associated_[position]=value;
  return position;
}
/* Sets rowLower (if row does not exist then
   all rows up to this are defined with default values and no elements)
*/
void 
CoinModel::setRowLower(int whichRow,double rowLower)
{
  assert (whichRow>=0);
  // make sure enough room and fill
  fillRows(whichRow,true);
  rowLower_[whichRow]=rowLower;
  rowType_[whichRow] &= ~1;
}
/* Sets rowUpper (if row does not exist then
   all rows up to this are defined with default values and no elements)
*/
void 
CoinModel::setRowUpper(int whichRow,double rowUpper)
{
  assert (whichRow>=0);
  // make sure enough room and fill
  fillRows(whichRow,true);
  rowUpper_[whichRow]=rowUpper;
  rowType_[whichRow] &= ~2;
}
/* Sets rowLower and rowUpper (if row does not exist then
   all rows up to this are defined with default values and no elements)
*/
void 
CoinModel::setRowBounds(int whichRow,double rowLower,double rowUpper)
{
  assert (whichRow>=0);
  // make sure enough room and fill
  fillRows(whichRow,true);
  rowLower_[whichRow]=rowLower;
  rowUpper_[whichRow]=rowUpper;
  rowType_[whichRow] &= ~3;
}
/* Sets name (if row does not exist then
   all rows up to this are defined with default values and no elements)
*/
void 
CoinModel::setRowName(int whichRow,const char * rowName)
{
  assert (whichRow>=0);
  // make sure enough room and fill
  fillRows(whichRow,true);
  assert (!noNames_) ;
  const char * oldName = rowName_.name(whichRow);
  if (oldName)
    rowName_.deleteHash(whichRow);
  if (rowName)
    rowName_.addHash(whichRow,rowName);
}
/* Sets columnLower (if column does not exist then
   all columns up to this are defined with default values and no elements)
*/
void 
CoinModel::setColumnLower(int whichColumn,double columnLower)
{
  assert (whichColumn>=0);
  // make sure enough room and fill
  fillColumns(whichColumn,true);
  columnLower_[whichColumn]=columnLower;
  columnType_[whichColumn] &= ~1;
}
/* Sets columnUpper (if column does not exist then
   all columns up to this are defined with default values and no elements)
*/
void 
CoinModel::setColumnUpper(int whichColumn,double columnUpper)
{
  assert (whichColumn>=0);
  // make sure enough room and fill
  fillColumns(whichColumn,true);
  columnUpper_[whichColumn]=columnUpper;
  columnType_[whichColumn] &= ~2;
}
/* Sets columnLower and columnUpper (if column does not exist then
   all columns up to this are defined with default values and no elements)
*/
void 
CoinModel::setColumnBounds(int whichColumn,double columnLower,double columnUpper)
{
  assert (whichColumn>=0);
  // make sure enough room and fill
  fillColumns(whichColumn,true);
  columnLower_[whichColumn]=columnLower;
  columnUpper_[whichColumn]=columnUpper;
  columnType_[whichColumn] &= ~3;
}
/* Sets columnObjective (if column does not exist then
   all columns up to this are defined with default values and no elements)
*/
void 
CoinModel::setColumnObjective(int whichColumn,double columnObjective)
{
  assert (whichColumn>=0);
  // make sure enough room and fill
  fillColumns(whichColumn,true);
  objective_[whichColumn]=columnObjective;
  columnType_[whichColumn] &= ~4;
}
/* Sets name (if column does not exist then
   all columns up to this are defined with default values and no elements)
*/
void 
CoinModel::setColumnName(int whichColumn,const char * columnName)
{
  assert (whichColumn>=0);
  // make sure enough room and fill
  fillColumns(whichColumn,true);
  const char * oldName = columnName_.name(whichColumn);
  assert (!noNames_) ;
  if (oldName)
    columnName_.deleteHash(whichColumn);
  if (columnName)
    columnName_.addHash(whichColumn,columnName);
}
/* Sets integer (if column does not exist then
   all columns up to this are defined with default values and no elements)
*/
void 
CoinModel::setColumnIsInteger(int whichColumn,bool columnIsInteger)
{
  assert (whichColumn>=0);
  // make sure enough room and fill
  fillColumns(whichColumn,true);
  integerType_[whichColumn]=(columnIsInteger) ? 1 : 0;
  columnType_[whichColumn] &= ~8;
}
// Adds one string, returns index
int 
CoinModel::addString(const char * string)
{
  int position = string_.hash(string);
  if (position<0) {
    position = string_.numberItems();
    string_.addHash(position,string);
  }
  return position;
}
/* Sets rowLower (if row does not exist then
   all rows up to this are defined with default values and no elements)
*/
void 
CoinModel::setRowLower(int whichRow,const char * rowLower) 
{
  assert (whichRow>=0);
  // make sure enough room and fill
  fillRows(whichRow,true);
  if (rowLower) {
    int value = addString(rowLower);
    rowLower_[whichRow]=value;
    rowType_[whichRow] |= 1; 
  } else {
    rowLower_[whichRow]=-COIN_DBL_MAX;
  }
}
/* Sets rowUpper (if row does not exist then
   all rows up to this are defined with default values and no elements)
*/
void 
CoinModel::setRowUpper(int whichRow,const char * rowUpper) 
{
  assert (whichRow>=0);
  // make sure enough room and fill
  fillRows(whichRow,true);
  if (rowUpper) {
    int value = addString(rowUpper);
    rowUpper_[whichRow]=value;
    rowType_[whichRow] |= 2; 
  } else {
    rowUpper_[whichRow]=COIN_DBL_MAX;
  }
}
/* Sets columnLower (if column does not exist then
   all columns up to this are defined with default values and no elements)
*/
void 
CoinModel::setColumnLower(int whichColumn,const char * columnLower) 
{
  assert (whichColumn>=0);
  // make sure enough room and fill
  fillColumns(whichColumn,true);
  if (columnLower) {
    int value = addString(columnLower);
    columnLower_[whichColumn]=value;
    columnType_[whichColumn] |= 1; 
  } else {
    columnLower_[whichColumn]=0.0;
  }
}
/* Sets columnUpper (if column does not exist then
   all columns up to this are defined with default values and no elements)
*/
void 
CoinModel::setColumnUpper(int whichColumn,const char * columnUpper) 
{
  assert (whichColumn>=0);
  // make sure enough room and fill
  fillColumns(whichColumn,true);
  if (columnUpper) {
    int value = addString(columnUpper);
    columnUpper_[whichColumn]=value;
    columnType_[whichColumn] |= 2; 
  } else {
    columnUpper_[whichColumn]=COIN_DBL_MAX;
  }
}
/* Sets columnObjective (if column does not exist then
   all columns up to this are defined with default values and no elements)
*/
void 
CoinModel::setColumnObjective(int whichColumn,const char * columnObjective) 
{
  assert (whichColumn>=0);
  // make sure enough room and fill
  fillColumns(whichColumn,true);
  if (columnObjective) {
    int value = addString(columnObjective);
    objective_[whichColumn]=value;
    columnType_[whichColumn] |= 4; 
  } else {
    objective_[whichColumn]=0.0;
  }
}
/* Sets integer (if column does not exist then
   all columns up to this are defined with default values and no elements)
*/
void 
CoinModel::setColumnIsInteger(int whichColumn,const char * columnIsInteger) 
{
  assert (whichColumn>=0);
  // make sure enough room and fill
  fillColumns(whichColumn,true);
  if (columnIsInteger) {
    int value = addString(columnIsInteger);
    integerType_[whichColumn]=value;
    columnType_[whichColumn] |= 8; 
  } else {
    integerType_[whichColumn]=0;
  }
}
//static const char * minusInfinity="-infinity";
//static const char * plusInfinity="+infinity";
//static const char * zero="0.0";
static const char * numeric="Numeric";
/* Gets rowLower (if row does not exist then -COIN_DBL_MAX)
 */
const char *  
CoinModel::getRowLowerAsString(int whichRow) const  
{
  assert (whichRow>=0);
  if (whichRow<numberRows_&&rowLower_) {
    if ((rowType_[whichRow]&1)!=0) {
      int position = static_cast<int> (rowLower_[whichRow]);
      return string_.name(position);
    } else {
      return numeric;
    }
  } else {
    return numeric;
  }
}
/* Gets rowUpper (if row does not exist then +COIN_DBL_MAX)
 */
const char *  
CoinModel::getRowUpperAsString(int whichRow) const  
{
  assert (whichRow>=0);
  if (whichRow<numberRows_&&rowUpper_) {
    if ((rowType_[whichRow]&2)!=0) {
      int position = static_cast<int> (rowUpper_[whichRow]);
      return string_.name(position);
    } else {
      return numeric;
    }
  } else {
    return numeric;
  }
}
/* Gets columnLower (if column does not exist then 0.0)
 */
const char *  
CoinModel::getColumnLowerAsString(int whichColumn) const  
{
  assert (whichColumn>=0);
  if (whichColumn<numberColumns_&&columnLower_) {
    if ((columnType_[whichColumn]&1)!=0) {
      int position = static_cast<int> (columnLower_[whichColumn]);
      return string_.name(position);
    } else {
      return numeric;
    }
  } else {
    return numeric;
  }
}
/* Gets columnUpper (if column does not exist then COIN_DBL_MAX)
 */
const char *  
CoinModel::getColumnUpperAsString(int whichColumn) const  
{
  assert (whichColumn>=0);
  if (whichColumn<numberColumns_&&columnUpper_) {
    if ((columnType_[whichColumn]&2)!=0) {
      int position = static_cast<int> (columnUpper_[whichColumn]);
      return string_.name(position);
    } else {
      return numeric;
    }
  } else {
    return numeric;
  }
}
/* Gets columnObjective (if column does not exist then 0.0)
 */
const char *  
CoinModel::getColumnObjectiveAsString(int whichColumn) const  
{
  assert (whichColumn>=0);
  if (whichColumn<numberColumns_&&objective_) {
    if ((columnType_[whichColumn]&4)!=0) {
      int position = static_cast<int> (objective_[whichColumn]);
      return string_.name(position);
    } else {
      return numeric;
    }
  } else {
    return numeric;
  }
}
/* Gets if integer (if column does not exist then false)
 */
const char * 
CoinModel::getColumnIsIntegerAsString(int whichColumn) const  
{
  assert (whichColumn>=0);
  if (whichColumn<numberColumns_&&integerType_) {
    if ((columnType_[whichColumn]&8)!=0) {
      int position = integerType_[whichColumn];
      return string_.name(position);
    } else {
      return numeric;
    }
  } else {
    return numeric;
  }
}
/* Deletes all entries in row and bounds.*/
void
CoinModel::deleteRow(int whichRow)
{
  assert (whichRow>=0);
  if (whichRow<numberRows_) {
    if (rowLower_) {
      rowLower_[whichRow]=-COIN_DBL_MAX;
      rowUpper_[whichRow]=COIN_DBL_MAX;
      rowType_[whichRow]=0;
      if (!noNames_) 
	rowName_.deleteHash(whichRow);
    }
    // need lists
    if (type_==0) {
      assert (start_);
      assert (!hashElements_.numberItems());
      delete [] start_;
      start_=NULL;
    }
    if ((links_&1)==0) {
      createList(1);
    } 
    assert (links_);
    // row links guaranteed to exist
    rowList_.deleteSame(whichRow,elements_,hashElements_,(links_!=3));
    // Just need to set first and last and take out
    if (links_==3)
      columnList_.updateDeleted(whichRow,elements_,rowList_);
  }
}
/* Deletes all entries in column and bounds.*/
void
CoinModel::deleteColumn(int whichColumn)
{
  assert (whichColumn>=0);
  if (whichColumn<numberColumns_) {
    if (columnLower_) {
      columnLower_[whichColumn]=0.0;
      columnUpper_[whichColumn]=COIN_DBL_MAX;
      objective_[whichColumn]=0.0;
      integerType_[whichColumn]=0;
      columnType_[whichColumn]=0;
      if (!noNames_) 
	columnName_.deleteHash(whichColumn);
    }
    // need lists
    if (type_==0) {
      assert (start_);
      assert (!hashElements_.numberItems());
      delete [] start_;
      start_=NULL;
    } else if (type_==3) {
      badType();
    }
    if ((links_&2)==0) {
      createList(2);
    } 
    assert (links_);
    // column links guaranteed to exist
    columnList_.deleteSame(whichColumn,elements_,hashElements_,(links_!=3));
    // Just need to set first and last and take out
    if (links_==3)
      rowList_.updateDeleted(whichColumn,elements_,columnList_);
  }
}
// Takes element out of matrix
int
CoinModel::deleteElement(int row, int column)
{
  int iPos = position(row,column);
  if (iPos>=0)
    deleteThisElement(row,column,iPos);
  return iPos;
}
// Takes element out of matrix when position known
void
#ifndef NDEBUG
CoinModel::deleteThisElement(int row, int column,int position)
#else
CoinModel::deleteThisElement(int , int ,int position)
#endif
{
  assert (row<numberRows_&&column<numberColumns_);
  assert (row==rowInTriple(elements_[position])&&
	  column==static_cast<int>(elements_[position].column));
  if ((links_&1)==0) {
    createList(1);
  } 
  assert (links_);
  // row links guaranteed to exist
  rowList_.deleteRowOne(position,elements_,hashElements_);
  // Just need to set first and last and take out
  if (links_==3)
    columnList_.updateDeletedOne(position,elements_);
  elements_[position].column=-1;
  elements_[position].value=0.0;
}
/* Packs down all rows i.e. removes empty rows permanently.  Empty rows
   have no elements and feasible bounds. returns number of rows deleted. */
int 
CoinModel::packRows()
{
  if (type_==3) 
    badType();
  int * newRow = new int[numberRows_];
  memset(newRow,0,numberRows_*sizeof(int));
  int iRow;
  int n=0;
  for (iRow=0;iRow<numberRows_;iRow++) {
    if (rowLower_[iRow]!=-COIN_DBL_MAX)
      newRow[iRow]++;
    if (rowUpper_[iRow]!=COIN_DBL_MAX)
      newRow[iRow]++;
    if (!noNames_&&rowName_.name(iRow))
	newRow[iRow]++;
  }
  int i;
  for ( i=0;i<numberElements_;i++) {
    if (elements_[i].column>=0) {
      iRow = rowInTriple(elements_[i]);
      assert (iRow>=0&&iRow<numberRows_);
      newRow[iRow]++;
    }
  }
  bool doRowNames = ( rowName_.numberItems() != 0 );
  for (iRow=0;iRow<numberRows_;iRow++) {
    if (newRow[iRow]) {
      rowLower_[n]=rowLower_[iRow];
      rowUpper_[n]=rowUpper_[iRow];
      rowType_[n]=rowType_[iRow];
      if (doRowNames)
        rowName_.setName(n,rowName_.getName(iRow));
      newRow[iRow]=n++;
    } else {
      newRow[iRow]=-1;
    }
  }
  int numberDeleted = numberRows_-n;
  if (numberDeleted) {
    numberRows_=n;
    n=0;
    for ( i=0;i<numberElements_;i++) {
      if (elements_[i].column>=0) {
        elements_[n]=elements_[i];
        setRowInTriple(elements_[n],newRow[rowInTriple(elements_[i])]);
        n++;
      }
    }
    numberElements_=n;
    // now redo
    if (doRowNames) {
      rowName_.setNumberItems(numberRows_);
      rowName_.resize(rowName_.maximumItems(),true);
    }
    if (hashElements_.numberItems()) {
      hashElements_.setNumberItems(numberElements_);
      hashElements_.resize(hashElements_.maximumItems(),elements_,true);
    }
    if (start_) {
      int last=-1;
      if (type_==0) {
        for (i=0;i<numberElements_;i++) {
          int now = rowInTriple(elements_[i]);
          assert (now>=last);
          if (now>last) {
            start_[last+1]=numberElements_;
            for (int j=last+1;j<now;j++)
              start_[j+1]=numberElements_;
            last=now;
          }
        }
        for (int j=last+1;j<numberRows_;j++)
          start_[j+1]=numberElements_;
      } else {
        assert (type_==1);
        for (i=0;i<numberElements_;i++) {
          int now = elements_[i].column;
          assert (now>=last);
          if (now>last) {
            start_[last+1]=numberElements_;
            for (int j=last+1;j<now;j++)
              start_[j+1]=numberElements_;
            last=now;
          }
        }
        for (int j=last+1;j<numberColumns_;j++)
          start_[j+1]=numberElements_;
      }
    }
    if ((links_&1)!=0) {
      rowList_ = CoinModelLinkedList();
      links_ &= ~1;
      createList(1);
    }
    if ((links_&2)!=0) {
      columnList_ = CoinModelLinkedList();
      links_ &= ~2;
      createList(2);
    }
  }
  delete [] newRow;
  return numberDeleted;
}
/* Packs down all columns i.e. removes empty columns permanently.  Empty columns
   have no elements and no objective. returns number of columns deleted. */
int 
CoinModel::packColumns()
{
  if (type_==3) 
    badType();
  int * newColumn = new int[numberColumns_];
  memset(newColumn,0,numberColumns_*sizeof(int));
  int iColumn;
  int n=0;
  for (iColumn=0;iColumn<numberColumns_;iColumn++) {
    if (columnLower_[iColumn]!=0.0)
      newColumn[iColumn]++;
    if (columnUpper_[iColumn]!=COIN_DBL_MAX)
      newColumn[iColumn]++;
    if (objective_[iColumn]!=0.0)
      newColumn[iColumn]++;
    if (!noNames_&&columnName_.name(iColumn))
      newColumn[iColumn]++;
  }
  int i;
  for ( i=0;i<numberElements_;i++) {
    if (elements_[i].column>=0) {
      iColumn = static_cast<int> (elements_[i].column);
      assert (iColumn>=0&&iColumn<numberColumns_);
      newColumn[iColumn]++;
    }
  }
  bool doColumnNames = ( columnName_.numberItems() != 0 );
  for (iColumn=0;iColumn<numberColumns_;iColumn++) {
    if (newColumn[iColumn]) {
      columnLower_[n]=columnLower_[iColumn];
      columnUpper_[n]=columnUpper_[iColumn];
      objective_[n]=objective_[iColumn];
      integerType_[n]=integerType_[iColumn];
      columnType_[n]=columnType_[iColumn];
      if (doColumnNames)
        columnName_.setName(n,columnName_.getName(iColumn));
      newColumn[iColumn]=n++;
    } else {
      newColumn[iColumn]=-1;
    }
  }
  int numberDeleted = numberColumns_-n;
  if (numberDeleted) {
    numberColumns_=n;
    n=0;
    for ( i=0;i<numberElements_;i++) {
      if (elements_[i].column>=0) {
        elements_[n]=elements_[i];
        elements_[n].column = newColumn[elements_[i].column];
        n++;
      }
    }
    numberElements_=n;
    // now redo
    if (doColumnNames) {
      columnName_.setNumberItems(numberColumns_);
      columnName_.resize(columnName_.maximumItems(),true);
    }
    if (hashElements_.numberItems()) {
      hashElements_.setNumberItems(numberElements_);
      hashElements_.resize(hashElements_.maximumItems(),elements_,true);
    }
    if (start_) {
      int last=-1;
      if (type_==0) {
        for (i=0;i<numberElements_;i++) {
          int now = rowInTriple(elements_[i]);
          assert (now>=last);
          if (now>last) {
            start_[last+1]=numberElements_;
            for (int j=last+1;j<now;j++)
              start_[j+1]=numberElements_;
            last=now;
          }
        }
        for (int j=last+1;j<numberRows_;j++)
          start_[j+1]=numberElements_;
      } else {
        assert (type_==1);
        for (i=0;i<numberElements_;i++) {
          int now = elements_[i].column;
          assert (now>=last);
          if (now>last) {
            start_[last+1]=numberElements_;
            for (int j=last+1;j<now;j++)
              start_[j+1]=numberElements_;
            last=now;
          }
        }
        for (int j=last+1;j<numberColumns_;j++)
          start_[j+1]=numberElements_;
      }
    }
    if ((links_&1)!=0) {
      rowList_ = CoinModelLinkedList();
      links_ &= ~1;
      createList(1);
    }
    if ((links_&2)!=0) {
      columnList_ = CoinModelLinkedList();
      links_ &= ~2;
      createList(2);
    }
  }
  delete [] newColumn;
  return numberDeleted;
}
/* Packs down all rows and columns.  i.e. removes empty rows and columns permanently.
   Empty rows have no elements and feasible bounds.
   Empty columns have no elements and no objective.
   returns number of rows+columns deleted. */
int 
CoinModel::pack()
{
  // For now do slowly (obvious overheads)
  return packRows()+packColumns();
}
// Creates a packed matrix - return sumber of errors
int 
CoinModel::createPackedMatrix(CoinPackedMatrix & matrix, 
			      const double * associated)
{
  if (type_==3) 
    return 0; // badType();
  // Set to say all parts
  type_=2;
  resize(numberRows_,numberColumns_,numberElements_);
  // Do counts for CoinPackedMatrix
  int * length = new int[numberColumns_];
  CoinZeroN(length,numberColumns_);
  int i;
  int numberElements=0;
  for (i=0;i<numberElements_;i++) {
    int column = elements_[i].column;
    if (column>=0) {
      length[column]++;
      numberElements++;
    }
  }
  int numberErrors=0;
  CoinBigIndex * start = new int[numberColumns_+1];
  int * row = new int[numberElements];
  double * element = new double[numberElements];
  start[0]=0;
  for (i=0;i<numberColumns_;i++) {
    start[i+1]=start[i]+length[i];
    length[i]=0;
  }
  numberElements=0;
  for (i=0;i<numberElements_;i++) {
    int column = elements_[i].column;
    if (column>=0) {
      double value = elements_[i].value;
      if (stringInTriple(elements_[i])) {
        int position = static_cast<int> (value);
        assert (position<sizeAssociated_);
        value = associated[position];
        if (value==unsetValue()) {
          numberErrors++;
          value=0.0;
        }
      }
      if (value) {
        numberElements++;
        int put=start[column]+length[column];
        row[put]=rowInTriple(elements_[i]);
        element[put]=value;
        length[column]++;
      }
    }
  }
  for (i=0;i<numberColumns_;i++) {
    int put = start[i];
    CoinSort_2(row+put,row+put+length[i],element+put);
  }
  matrix=CoinPackedMatrix(true,numberRows_,numberColumns_,numberElements,
                          element,row,start,length,0.0,0.0);
  delete [] start;
  delete [] length;
  delete [] row;
  delete [] element;
  return numberErrors;
}
/* Fills in startPositive and startNegative with counts for +-1 matrix.
   If not +-1 then startPositive[0]==-1 otherwise counts and
   startPositive[numberColumns]== size
      - return number of errors
*/
int 
CoinModel::countPlusMinusOne(CoinBigIndex * startPositive, CoinBigIndex * startNegative,
                             const double * associated)
{
  if (type_==3) 
    badType();
  memset(startPositive,0,numberColumns_*sizeof(int));
  memset(startNegative,0,numberColumns_*sizeof(int));
  // Set to say all parts
  type_=2;
  resize(numberRows_,numberColumns_,numberElements_);
  int numberErrors=0;
  CoinBigIndex numberElements=0;
  for (CoinBigIndex i=0;i<numberElements_;i++) {
    int column = elements_[i].column;
    if (column>=0) {
      double value = elements_[i].value;
      if (stringInTriple(elements_[i])) {
        int position = static_cast<int> (value);
        assert (position<sizeAssociated_);
        value = associated[position];
        if (value==unsetValue()) {
          numberErrors++;
          value=0.0;
          startPositive[0]=-1;
          break;
        }
      }
      if (value) {
        numberElements++;
        if (value==1.0) {
          startPositive[column]++;
        } else if (value==-1.0) {
          startNegative[column]++;
        } else {
          startPositive[0]=-1;
          break;
        }
      }
    }
  }
  if (startPositive[0]>=0) 
    startPositive[numberColumns_]=numberElements;
  return numberErrors;
}
/* Creates +-1 matrix given startPositive and startNegative counts for +-1 matrix.
 */
void 
CoinModel::createPlusMinusOne(CoinBigIndex * startPositive, CoinBigIndex * startNegative,
                              int * indices,
                              const double * associated)
{
  if (type_==3) 
    badType();
  CoinBigIndex size=0;
  int iColumn;
  for (iColumn=0;iColumn<numberColumns_;iColumn++) {
    CoinBigIndex n=startPositive[iColumn];
    startPositive[iColumn]=size;
    size+= n;
    n=startNegative[iColumn];
    startNegative[iColumn]=size;
    size+= n;
  }
  startPositive[numberColumns_]=size;
  for (CoinBigIndex i=0;i<numberElements_;i++) {
    int column = elements_[i].column;
    if (column>=0) {
      double value = elements_[i].value;
      if (stringInTriple(elements_[i])) {
        int position = static_cast<int> (value);
        assert (position<sizeAssociated_);
        value = associated[position];
      }
      int iRow=rowInTriple(elements_[i]);
      if (value==1.0) {
        CoinBigIndex position = startPositive[column];
        indices[position]=iRow;
        startPositive[column]++;
      } else if (value==-1.0) {
        CoinBigIndex position = startNegative[column];
        indices[position]=iRow;
        startNegative[column]++;
      }
    }
  }
  // and now redo starts
  for (iColumn=numberColumns_-1;iColumn>=0;iColumn--) {
    startPositive[iColumn+1]=startNegative[iColumn];
    startNegative[iColumn]=startPositive[iColumn];
  }
  startPositive[0]=0;
  for (iColumn=0;iColumn<numberColumns_;iColumn++) {
    CoinBigIndex start = startPositive[iColumn];
    CoinBigIndex end = startNegative[iColumn];
    std::sort(indices+start,indices+end);
    start = startNegative[iColumn];
    end = startPositive[iColumn+1];
    std::sort(indices+start,indices+end);
  }
}
// Fills in all associated - returning number of errors
int CoinModel::computeAssociated(double * associated)
{
  CoinYacc info;
  info.length=0;
  int numberErrors=0;
  for (int i=0;i<string_.numberItems();i++) {
    if (string_.name(i)&&associated[i]==unsetValue()) {
      associated[i] = getDoubleFromString(info,string_.name(i));
      if (associated[i]==unsetValue())
        numberErrors++;
    }
  }
  return numberErrors;
}
// Creates copies of various arrays - return number of errors
int 
CoinModel::createArrays(double * & rowLower, double * &  rowUpper,
                        double * & columnLower, double * & columnUpper,
                        double * & objective, int * & integerType,
                        double * & associated)
{
  if (sizeAssociated_<string_.numberItems()) {
    int newSize = string_.numberItems();
    double * temp = new double[newSize];
    CoinMemcpyN(associated_,sizeAssociated_,temp);
    CoinFillN(temp+sizeAssociated_,newSize-sizeAssociated_,unsetValue());
    delete [] associated_;
    associated_ = temp;
    sizeAssociated_=newSize;
  }
  associated = CoinCopyOfArray(associated_,sizeAssociated_);
  int numberErrors = computeAssociated(associated);
  // Fill in as much as possible
  rowLower = CoinCopyOfArray( rowLower_,numberRows_);
  rowUpper = CoinCopyOfArray( rowUpper_,numberRows_);
  for (int iRow=0;iRow<numberRows_;iRow++) {
    if ((rowType_[iRow]&1)!=0) {
      int position = static_cast<int> (rowLower[iRow]);
      assert (position<sizeAssociated_);
      double value = associated[position];
      if (value!=unsetValue()) {
        rowLower[iRow]=value;
      }
    }
    if ((rowType_[iRow]&2)!=0) {
      int position = static_cast<int> (rowUpper[iRow]);
      assert (position<sizeAssociated_);
      double value = associated[position];
      if (value!=unsetValue()) {
        rowUpper[iRow]=value;
      }
    }
  }
  columnLower = CoinCopyOfArray( columnLower_,numberColumns_);
  columnUpper = CoinCopyOfArray( columnUpper_,numberColumns_);
  objective = CoinCopyOfArray( objective_,numberColumns_);
  integerType = CoinCopyOfArray( integerType_,numberColumns_);
  for (int iColumn=0;iColumn<numberColumns_;iColumn++) {
    if ((columnType_[iColumn]&1)!=0) {
      int position = static_cast<int> (columnLower[iColumn]);
      assert (position<sizeAssociated_);
      double value = associated[position];
      if (value!=unsetValue()) {
        columnLower[iColumn]=value;
      }
    }
    if ((columnType_[iColumn]&2)!=0) {
      int position = static_cast<int> (columnUpper[iColumn]);
      assert (position<sizeAssociated_);
      double value = associated[position];
      if (value!=unsetValue()) {
        columnUpper[iColumn]=value;
      }
    }
    if ((columnType_[iColumn]&4)!=0) {
      int position = static_cast<int> (objective[iColumn]);
      assert (position<sizeAssociated_);
      double value = associated[position];
      if (value!=unsetValue()) {
        objective[iColumn]=value;
      }
    }
    if ((columnType_[iColumn]&8)!=0) {
      int position = integerType[iColumn];
      assert (position<sizeAssociated_);
      double value = associated[position];
      if (value!=unsetValue()) {
        integerType[iColumn]=static_cast<int> (value);
      }
    }
  }
  return numberErrors;
}

/* Write the problem in MPS format to a file with the given filename.
 */
int 
CoinModel::writeMps(const char *filename, int compression,
                    int formatType , int numberAcross , bool keepStrings) 
{
  int numberErrors = 0;
  // Set arrays for normal use
  double * rowLower = rowLower_;
  double * rowUpper = rowUpper_;
  double * columnLower = columnLower_;
  double * columnUpper = columnUpper_;
  double * objective = objective_;
  int * integerType = integerType_;
  double * associated = associated_;
  // If strings then do copies
  if (string_.numberItems()) {
    numberErrors = createArrays(rowLower, rowUpper, columnLower, columnUpper,
                                 objective, integerType,associated);
  }
  CoinPackedMatrix matrix;
  if (type_!=3) {
    createPackedMatrix(matrix,associated);
  } else {
    matrix = *packedMatrix_;
  }
  char* integrality = new char[numberColumns_];
  bool hasInteger = false;
  for (int i = 0; i < numberColumns_; i++) {
    if (integerType[i]) {
      integrality[i] = 1;
      hasInteger = true;
    } else {
      integrality[i] = 0;
    }
  }

  CoinMpsIO writer;
  writer.setInfinity(COIN_DBL_MAX);
  const char *const * rowNames=NULL;
  if (rowName_.numberItems())
    rowNames=rowName_.names();
  const char * const * columnNames=NULL;
  if (columnName_.numberItems())
    columnNames=columnName_.names();
  writer.setMpsData(matrix, COIN_DBL_MAX,
                    columnLower, columnUpper,
                    objective, hasInteger ? integrality : 0,
		     rowLower, rowUpper,
		     columnNames,rowNames);
  delete[] integrality;
  if (rowLower!=rowLower_) {
    delete [] rowLower;
    delete [] rowUpper;
    delete [] columnLower;
    delete [] columnUpper;
    delete [] objective;
    delete [] integerType;
    delete [] associated;
    if (numberErrors&&logLevel_>0&&!keepStrings)
      printf("%d string elements had no values associated with them\n",numberErrors);
  }
  writer.setObjectiveOffset(objectiveOffset_);
  writer.setProblemName(problemName_.c_str());
  if (keepStrings&&string_.numberItems()) {
    // load up strings - sorted by column and row
    writer.copyStringElements(this);
  }
  return writer.writeMps(filename, compression, formatType, numberAcross);
}
/* Check two models against each other.  Return nonzero if different.
   Ignore names if that set.
   May modify both models by cleaning up
*/
int 
CoinModel::differentModel(CoinModel & other, bool ignoreNames)
{
  int numberErrors = 0;
  int numberErrors2 = 0;
  int returnCode=0;
  if (numberRows_!=other.numberRows_||numberColumns_!=other.numberColumns_) {
    if (logLevel_>0)
      printf("** Mismatch on size, this has %d rows, %d columns - other has %d rows, %d columns\n",
             numberRows_,numberColumns_,other.numberRows_,other.numberColumns_);
    returnCode=1000;
  }
  // Set arrays for normal use
  double * rowLower = rowLower_;
  double * rowUpper = rowUpper_;
  double * columnLower = columnLower_;
  double * columnUpper = columnUpper_;
  double * objective = objective_;
  int * integerType = integerType_;
  double * associated = associated_;
  // If strings then do copies
  if (string_.numberItems()) {
    numberErrors += createArrays(rowLower, rowUpper, columnLower, columnUpper,
                                 objective, integerType,associated);
  }
  // Set arrays for normal use
  double * rowLower2 = other.rowLower_;
  double * rowUpper2 = other.rowUpper_;
  double * columnLower2 = other.columnLower_;
  double * columnUpper2 = other.columnUpper_;
  double * objective2 = other.objective_;
  int * integerType2 = other.integerType_;
  double * associated2 = other.associated_;
  // If strings then do copies
  if (other.string_.numberItems()) {
    numberErrors2 += other.createArrays(rowLower2, rowUpper2, columnLower2, columnUpper2,
                                 objective2, integerType2,associated2);
  }
  CoinPackedMatrix matrix;
  createPackedMatrix(matrix,associated);
  CoinPackedMatrix matrix2;
  other.createPackedMatrix(matrix2,associated2);
  if (numberErrors||numberErrors2)
    if (logLevel_>0)
      printf("** Errors when converting strings, %d on this, %d on other\n",
             numberErrors,numberErrors2);
  CoinRelFltEq tolerance;
  if (numberRows_==other.numberRows_) {
    bool checkNames = !ignoreNames;
    if (!rowName_.numberItems()||
        !other.rowName_.numberItems())
      checkNames=false;
    int numberDifferentL = 0;
    int numberDifferentU = 0;
    int numberDifferentN = 0;
    for (int i=0;i<numberRows_;i++) {
      if (!tolerance(rowLower[i],rowLower2[i]))
        numberDifferentL++;
      if (!tolerance(rowUpper[i],rowUpper2[i]))
        numberDifferentU++;
      if (checkNames&&rowName_.name(i)&&other.rowName_.name(i)) {
        if (strcmp(rowName_.name(i),other.rowName_.name(i)))
          numberDifferentN++;
      }
    }
    int n = numberDifferentL+numberDifferentU+numberDifferentN;
    returnCode+=n;
    if (n&&logLevel_>0)
      printf("Row differences , %d lower, %d upper and %d names\n",
             numberDifferentL,numberDifferentU,numberDifferentN);
  }
  if (numberColumns_==other.numberColumns_) {
    int numberDifferentL = 0;
    int numberDifferentU = 0;
    int numberDifferentN = 0;
    int numberDifferentO = 0;
    int numberDifferentI = 0;
    bool checkNames = !ignoreNames;
    if (!columnName_.numberItems()||
        !other.columnName_.numberItems())
      checkNames=false;
    for (int i=0;i<numberColumns_;i++) {
      if (!tolerance(columnLower[i],columnLower2[i]))
        numberDifferentL++;
      if (!tolerance(columnUpper[i],columnUpper2[i]))
        numberDifferentU++;
      if (!tolerance(objective[i],objective2[i]))
        numberDifferentO++;
      int i1 = (integerType) ? integerType[i] : 0;
      int i2 = (integerType2) ? integerType2[i] : 0;
      if (i1!=i2)
        numberDifferentI++;
      if (checkNames&&columnName_.name(i)&&other.columnName_.name(i)) {
        if (strcmp(columnName_.name(i),other.columnName_.name(i)))
          numberDifferentN++;
      }
    }
    int n = numberDifferentL+numberDifferentU+numberDifferentN;
    n+= numberDifferentO+numberDifferentI;
    returnCode+=n;
    if (n&&logLevel_>0)
      printf("Column differences , %d lower, %d upper, %d objective, %d integer and %d names\n",
             numberDifferentL,numberDifferentU,numberDifferentO,
             numberDifferentI,numberDifferentN);
  }
  if (numberRows_==other.numberRows_&&
      numberColumns_==other.numberColumns_&&
      numberElements_==other.numberElements_) {
    if (!matrix.isEquivalent(matrix2,tolerance)) {
      returnCode+=100;
    if (returnCode&&logLevel_>0)
      printf("Two matrices are not same\n");
    }
  }

  if (rowLower!=rowLower_) {
    delete [] rowLower;
    delete [] rowUpper;
    delete [] columnLower;
    delete [] columnUpper;
    delete [] objective;
    delete [] integerType;
    delete [] associated;
  }
  if (rowLower2!=other.rowLower_) {
    delete [] rowLower2;
    delete [] rowUpper2;
    delete [] columnLower2;
    delete [] columnUpper2;
    delete [] objective2;
    delete [] integerType2;
    delete [] associated2;
  }
  return returnCode;
}
// Returns value for row i and column j
double 
CoinModel::getElement(int i,int j) const
{
  if (!hashElements_.numberItems()) {
    hashElements_.setNumberItems(numberElements_);
    hashElements_.resize(maximumElements_,elements_);
  }
  int position = hashElements_.hash(i,j,elements_);
  if (position>=0) {
    return elements_[position].value;
  } else {
    return 0.0;
  }
}
// Returns value for row rowName and column columnName
double 
CoinModel::getElement(const char * rowName,const char * columnName) const
{
  if (!hashElements_.numberItems()) {
    hashElements_.setNumberItems(numberElements_);
    hashElements_.resize(maximumElements_,elements_);
  }
  assert (!noNames_); 
  int i=rowName_.hash(rowName);
  int j=columnName_.hash(columnName);
  int position;
  if (i>=0&&j>=0)
    position = hashElements_.hash(i,j,elements_);
  else
    position=-1;
  if (position>=0) {
    return elements_[position].value;
  } else {
    return 0.0;
  }
}
// Returns quadratic value for columns i and j
double 
CoinModel::getQuadraticElement(int ,int ) const
{
  printf("not written yet\n");
  abort();
  return 0.0;
}
// Returns value for row i and column j as string
const char * 
CoinModel::getElementAsString(int i,int j) const
{
  if (!hashElements_.numberItems()) {
    hashElements_.setNumberItems(numberElements_);
    hashElements_.resize(maximumElements_,elements_);
  }
  int position = hashElements_.hash(i,j,elements_);
  if (position>=0) {
    if (stringInTriple(elements_[position])) {
      int iString =  static_cast<int> (elements_[position].value);
      assert (iString>=0&&iString<string_.numberItems());
      return string_.name(iString);
    } else {
      return numeric;
    }
  } else {
    return NULL;
  }
}
/* Returns position of element for row i column j.
   Only valid until next modification. 
   -1 if element does not exist */
int
CoinModel::position (int i,int j) const
{
  if (!hashElements_.numberItems()) {
    hashElements_.setNumberItems(numberElements_);
    hashElements_.resize(maximumElements_,elements_,true);
  }
  return hashElements_.hash(i,j,elements_);
}

/* Returns pointer to element for row i column j.
   Only valid until next modification. 
   NULL if element does not exist */
double * 
CoinModel::pointer (int i,int j) const
{
  if (!hashElements_.numberItems()) {
    hashElements_.setNumberItems(numberElements_);
    hashElements_.resize(maximumElements_,elements_);
  }
  int position = hashElements_.hash(i,j,elements_);
  if (position>=0) {
    return &(elements_[position].value);
  } else {
    return NULL;
  }
}

  
/* Returns first element in given row - index is -1 if none.
   Index is given by .index and value by .value
*/
CoinModelLink 
CoinModel::firstInRow(int whichRow) const
{
  CoinModelLink link;
  if (whichRow>=0&&whichRow<numberRows_) {
    link.setOnRow(true);
    if (type_==0) {
      assert (start_);
      int position = start_[whichRow];
      if (position<start_[whichRow+1]) {
        link.setRow(whichRow);
        link.setPosition(position);
        link.setColumn(elements_[position].column);
        assert (whichRow==rowInTriple(elements_[position]));
        link.setValue(elements_[position].value);
      }
    } else {
      fillList(whichRow,rowList_,1);
      int position = rowList_.first(whichRow);
      if (position>=0) {
        link.setRow(whichRow);
        link.setPosition(position);
        link.setColumn(elements_[position].column);
        assert (whichRow==rowInTriple(elements_[position]));
        link.setValue(elements_[position].value);
      }
    }
  }
  return link;
}
/* Returns last element in given row - index is -1 if none.
   Index is given by .index and value by .value
  */
CoinModelLink 
CoinModel::lastInRow(int whichRow) const
{
  CoinModelLink link;
  if (whichRow>=0&&whichRow<numberRows_) {
    link.setOnRow(true);
    if (type_==0) {
      assert (start_);
      int position = start_[whichRow+1]-1;
      if (position>=start_[whichRow]) {
        link.setRow(whichRow);
        link.setPosition(position);
        link.setColumn(elements_[position].column);
        assert (whichRow==rowInTriple(elements_[position]));
        link.setValue(elements_[position].value);
      }
    } else {
      fillList(whichRow,rowList_,1);
      int position = rowList_.last(whichRow);
      if (position>=0) {
        link.setRow(whichRow);
        link.setPosition(position);
        link.setColumn(elements_[position].column);
        assert (whichRow==rowInTriple(elements_[position]));
        link.setValue(elements_[position].value);
      }
    }
  }
  return link;
}
/* Returns first element in given column - index is -1 if none.
   Index is given by .index and value by .value
*/
CoinModelLink 
CoinModel::firstInColumn(int whichColumn) const
{
  CoinModelLink link;
  if (whichColumn>=0&&whichColumn<numberColumns_) {
    link.setOnRow(false);
    if (type_==1) {
      assert (start_);
      int position = start_[whichColumn];
      if (position<start_[whichColumn+1]) {
        link.setColumn(whichColumn);
        link.setPosition(position);
        link.setRow(rowInTriple(elements_[position]));
        assert (whichColumn==static_cast<int> (elements_[position].column));
        link.setValue(elements_[position].value);
      }
    } else {
      fillList(whichColumn,columnList_,2);
      if ((links_&2)==0) {
        // Create list
        assert (!columnList_.numberMajor());
        createList(2);
      }
      int position = columnList_.first(whichColumn);
      if (position>=0) {
        link.setColumn(whichColumn);
        link.setPosition(position);
        link.setRow(rowInTriple(elements_[position]));
        assert (whichColumn==static_cast<int> (elements_[position].column));
        link.setValue(elements_[position].value);
      }
    }
  }
  return link;
}
/* Returns last element in given column - index is -1 if none.
   Index is given by .index and value by .value
*/
CoinModelLink 
CoinModel::lastInColumn(int whichColumn) const
{
  CoinModelLink link;
  if (whichColumn>=0&&whichColumn<numberColumns_) {
    link.setOnRow(false);
    if (type_==1) {
      assert (start_);
      int position = start_[whichColumn+1]-1;
      if (position>=start_[whichColumn]) {
        link.setColumn(whichColumn);
        link.setPosition(position);
        link.setRow(rowInTriple(elements_[position]));
        assert (whichColumn==static_cast<int> (elements_[position].column));
        link.setValue(elements_[position].value);
      }
    } else {
      fillList(whichColumn,columnList_,2);
      int position = columnList_.last(whichColumn);
      if (position>=0) {
        link.setColumn(whichColumn);
        link.setPosition(position);
        link.setRow(rowInTriple(elements_[position]));
        assert (whichColumn==static_cast<int> (elements_[position].column));
        link.setValue(elements_[position].value);
      }
    }
  }
  return link;
}
/* Returns next element in current row or column - index is -1 if none.
   Index is given by .index and value by .value.
   User could also tell because input.next would be NULL
*/
CoinModelLink 
CoinModel::next(CoinModelLink & current) const
{
  CoinModelLink link=current;
  int position = current.position();
  if (position>=0) {
    if (current.onRow()) {
      // Doing by row
      int whichRow = current.row();
      if (type_==0) {
        assert (start_);
        position++;
        if (position<start_[whichRow+1]) {
          link.setPosition(position);
          link.setColumn(elements_[position].column);
          assert (whichRow==rowInTriple(elements_[position]));
          link.setValue(elements_[position].value);
        } else {
          // signal end
          link.setPosition(-1);
          link.setColumn(-1);
          link.setRow(-1);
          link.setValue(0.0);
        }
      } else {
        assert ((links_&1)!=0);
        position = rowList_.next()[position];
        if (position>=0) {
          link.setPosition(position);
          link.setColumn(elements_[position].column);
          assert (whichRow==rowInTriple(elements_[position]));
          link.setValue(elements_[position].value);
        } else {
          // signal end
          link.setPosition(-1);
          link.setColumn(-1);
          link.setRow(-1);
          link.setValue(0.0);
        }
      }
    } else {
      // Doing by column
      int whichColumn = current.column();
      if (type_==1) {
        assert (start_);
        position++;
        if (position<start_[whichColumn+1]) {
          link.setPosition(position);
          link.setRow(rowInTriple(elements_[position]));
          assert (whichColumn==static_cast<int> (elements_[position].column));
          link.setValue(elements_[position].value);
        } else {
          // signal end
          link.setPosition(-1);
          link.setColumn(-1);
          link.setRow(-1);
          link.setValue(0.0);
        }
      } else {
        assert ((links_&2)!=0);
        position = columnList_.next()[position];
        if (position>=0) {
          link.setPosition(position);
          link.setRow(rowInTriple(elements_[position]));
          assert (whichColumn==static_cast<int> (elements_[position].column));
          link.setValue(elements_[position].value);
        } else {
          // signal end
          link.setPosition(-1);
          link.setColumn(-1);
          link.setRow(-1);
          link.setValue(0.0);
        }
      }
    }
  }
  return link;
}
/* Returns previous element in current row or column - index is -1 if none.
   Index is given by .index and value by .value.
   User could also tell because input.previous would be NULL
*/
CoinModelLink 
CoinModel::previous(CoinModelLink & current) const
{
  CoinModelLink link=current;
  int position = current.position();
  if (position>=0) {
    if (current.onRow()) {
      // Doing by row
      int whichRow = current.row();
      if (type_==0) {
        assert (start_);
        position--;
        if (position>=start_[whichRow]) {
          link.setPosition(position);
          link.setColumn(elements_[position].column);
          assert (whichRow==rowInTriple(elements_[position]));
          link.setValue(elements_[position].value);
        } else {
          // signal end
          link.setPosition(-1);
          link.setColumn(-1);
          link.setRow(-1);
          link.setValue(0.0);
        }
      } else {
        assert ((links_&1)!=0);
        position = rowList_.previous()[position];
        if (position>=0) {
          link.setPosition(position);
          link.setColumn(elements_[position].column);
          assert (whichRow==rowInTriple(elements_[position]));
          link.setValue(elements_[position].value);
        } else {
          // signal end
          link.setPosition(-1);
          link.setColumn(-1);
          link.setRow(-1);
          link.setValue(0.0);
        }
      }
    } else {
      // Doing by column
      int whichColumn = current.column();
      if (type_==1) {
        assert (start_);
        position--;
        if (position>=start_[whichColumn]) {
          link.setPosition(position);
          link.setRow(rowInTriple(elements_[position]));
          assert (whichColumn==static_cast<int> (elements_[position].column));
          link.setValue(elements_[position].value);
        } else {
          // signal end
          link.setPosition(-1);
          link.setColumn(-1);
          link.setRow(-1);
          link.setValue(0.0);
        }
      } else {
        assert ((links_&2)!=0);
        position = columnList_.previous()[position];
        if (position>=0) {
          link.setPosition(position);
          link.setRow(rowInTriple(elements_[position]));
          assert (whichColumn==static_cast<int> (elements_[position].column));
          link.setValue(elements_[position].value);
        } else {
          // signal end
          link.setPosition(-1);
          link.setColumn(-1);
          link.setRow(-1);
          link.setValue(0.0);
        }
      }
    }
  }
  return link;
}
/* Returns first element in given quadratic column - index is -1 if none.
   Index is given by .index and value by .value
*/
CoinModelLink 
CoinModel::firstInQuadraticColumn(int ) const
{
  printf("not written yet\n");
  abort();
  CoinModelLink x;
  return x;
}
/* Returns last element in given quadratic column - index is -1 if none.
   Index is given by .index and value by .value
*/
CoinModelLink 
CoinModel::lastInQuadraticColumn(int) const
{
  printf("not written yet\n");
  abort();
  CoinModelLink x;
  return x;
}
/* Gets rowLower (if row does not exist then -COIN_DBL_MAX)
 */
double  
CoinModel::getRowLower(int whichRow) const
{
  assert (whichRow>=0);
  if (whichRow<numberRows_&&rowLower_)
    return rowLower_[whichRow];
  else
    return -COIN_DBL_MAX;
}
/* Gets rowUpper (if row does not exist then +COIN_DBL_MAX)
 */
double  
CoinModel::getRowUpper(int whichRow) const
{
  assert (whichRow>=0);
  if (whichRow<numberRows_&&rowUpper_)
    return rowUpper_[whichRow];
  else
    return COIN_DBL_MAX;
}
/* Gets name (if row does not exist then NULL)
 */
const char * 
CoinModel::getRowName(int whichRow) const
{
  assert (whichRow>=0);
  if (whichRow<rowName_.numberItems())
    return rowName_.name(whichRow);
  else
    return NULL;
}
/* Gets columnLower (if column does not exist then 0.0)
 */
double  
CoinModel::getColumnLower(int whichColumn) const
{
  assert (whichColumn>=0);
  if (whichColumn<numberColumns_&&columnLower_)
    return columnLower_[whichColumn];
  else
    return 0.0;
}
/* Gets columnUpper (if column does not exist then COIN_DBL_MAX)
 */
double  
CoinModel::getColumnUpper(int whichColumn) const
{
  assert (whichColumn>=0);
  if (whichColumn<numberColumns_&&columnUpper_)
    return columnUpper_[whichColumn];
  else
    return COIN_DBL_MAX;
}
/* Gets columnObjective (if column does not exist then 0.0)
 */
double  
CoinModel::getColumnObjective(int whichColumn) const
{
  assert (whichColumn>=0);
  if (whichColumn<numberColumns_&&objective_)
    return objective_[whichColumn];
  else
    return 0.0;
}
/* Gets name (if column does not exist then NULL)
 */
const char * 
CoinModel::getColumnName(int whichColumn) const
{
  assert (whichColumn>=0);
  if (whichColumn<columnName_.numberItems())
    return columnName_.name(whichColumn);
  else
    return NULL;
}
/* Gets if integer (if column does not exist then false)
 */
bool 
CoinModel::getColumnIsInteger(int whichColumn) const
{
  assert (whichColumn>=0);
  if (whichColumn<numberColumns_&&integerType_)
    return integerType_[whichColumn]!=0;
  else
    return false;
}
// Row index from row name (-1 if no names or no match)
int 
CoinModel::row(const char * rowName) const
{
  assert (!noNames_) ;
  return rowName_.hash(rowName);
}
// Column index from column name (-1 if no names or no match)
int 
CoinModel::column(const char * columnName) const
{
  assert (!noNames_) ;
  return columnName_.hash(columnName);
}
// Resize
void 
CoinModel::resize(int maximumRows, int maximumColumns, int maximumElements)
{
  maximumElements = CoinMax(maximumElements,maximumElements_);
  if (type_==0||type_==2) {
    // need to redo row stuff
    maximumRows = CoinMax(maximumRows,numberRows_);
    if (maximumRows>maximumRows_) {
      bool needFill = rowLower_==NULL;
      double * tempArray;
      tempArray = new double[maximumRows];
      CoinMemcpyN(rowLower_,numberRows_,tempArray);
#     ifdef ZEROFAULT
      memset(tempArray+numberRows_,0,(maximumRows-numberRows_)*sizeof(double)) ;
#     endif
      delete [] rowLower_;
      rowLower_=tempArray;
      tempArray = new double[maximumRows];
      CoinMemcpyN(rowUpper_,numberRows_,tempArray);
#     ifdef ZEROFAULT
      memset(tempArray+numberRows_,0,(maximumRows-numberRows_)*sizeof(double)) ;
#     endif
      delete [] rowUpper_;
      rowUpper_=tempArray;
      int * tempArray2;
      tempArray2 = new int[maximumRows];
      CoinMemcpyN(rowType_,numberRows_,tempArray2);
#     ifdef ZEROFAULT
      memset(tempArray2+numberRows_,0,(maximumRows-numberRows_)*sizeof(int)) ;
#     endif
      delete [] rowType_;
      rowType_=tempArray2;
      // resize hash
      if (!noNames_) 
	rowName_.resize(maximumRows);
      // If we have links we need to resize
      if ((links_&1)!=0) {
        rowList_.resize(maximumRows,maximumElements);
      }
      // If we have start then we need to resize that
      if (type_==0) {
        int * tempArray2;
        tempArray2 = new int[maximumRows+1];
#	ifdef ZEROFAULT
	memset(tempArray2,0,(maximumRows+1)*sizeof(int)) ;
#	endif
        if (start_) {
          CoinMemcpyN(start_,(numberRows_+1),tempArray2);
          delete [] start_;
        } else {
          tempArray2[0]=0;
        }
        start_=tempArray2;
      }
      maximumRows_=maximumRows;
      // Fill
      if (needFill) {
        int save=numberRows_-1;
        numberRows_=0;
        fillRows(save,true);
      }
    }
  } else if (type_==3) {
    badType();
  }
  if (type_==1||type_==2) {
    // need to redo column stuff
    maximumColumns = CoinMax(maximumColumns,numberColumns_);
    if (maximumColumns>maximumColumns_) {
      bool needFill = columnLower_==NULL;
      double * tempArray;
      tempArray = new double[maximumColumns];
      CoinMemcpyN(columnLower_,numberColumns_,tempArray);
#     ifdef ZEROFAULT
      memset(tempArray+numberColumns_,0,
	     (maximumColumns-numberColumns_)*sizeof(double)) ;
#     endif
      delete [] columnLower_;
      columnLower_=tempArray;
      tempArray = new double[maximumColumns];
      CoinMemcpyN(columnUpper_,numberColumns_,tempArray);
#     ifdef ZEROFAULT
      memset(tempArray+numberColumns_,0,
	     (maximumColumns-numberColumns_)*sizeof(double)) ;
#     endif
      delete [] columnUpper_;
      columnUpper_=tempArray;
      tempArray = new double[maximumColumns];
      CoinMemcpyN(objective_,numberColumns_,tempArray);
#     ifdef ZEROFAULT
      memset(tempArray+numberColumns_,0,
	     (maximumColumns-numberColumns_)*sizeof(double)) ;
#     endif
      delete [] objective_;
      objective_=tempArray;
      int * tempArray2;
      tempArray2 = new int[maximumColumns];
      CoinMemcpyN(columnType_,numberColumns_,tempArray2);
#     ifdef ZEROFAULT
      memset(tempArray2+numberColumns_,0,
	     (maximumColumns-numberColumns_)*sizeof(int)) ;
#     endif
      delete [] columnType_;
      columnType_=tempArray2;
      tempArray2 = new int[maximumColumns];
      CoinMemcpyN(integerType_,numberColumns_,tempArray2);
#     ifdef ZEROFAULT
      memset(tempArray2+numberColumns_,0,
	     (maximumColumns-numberColumns_)*sizeof(int)) ;
#     endif
      delete [] integerType_;
      integerType_=tempArray2;
      // resize hash
      if (!noNames_) 
	columnName_.resize(maximumColumns);
      // If we have links we need to resize
      if ((links_&2)!=0) {
        columnList_.resize(maximumColumns,maximumElements);
      }
      // If we have start then we need to resize that
      if (type_==1) {
        int * tempArray2;
        tempArray2 = new int[maximumColumns+1];
#       ifdef ZEROFAULT
        memset(tempArray2,0,(maximumColumns+1)*sizeof(int)) ;
#       endif
        if (start_) {
          CoinMemcpyN(start_,(numberColumns_+1),tempArray2);
          delete [] start_;
        } else {
          tempArray2[0]=0;
        }
        start_=tempArray2;
      }
      maximumColumns_=maximumColumns;
      // Fill
      if (needFill) {
        int save=numberColumns_-1;
        numberColumns_=0;
        fillColumns(save,true);
      }
    }
  }
  if (type_==3) 
    badType();
  if (maximumElements>maximumElements_) {
    CoinModelTriple * tempArray = new CoinModelTriple[maximumElements];
    CoinMemcpyN(elements_,numberElements_,tempArray);
#   ifdef ZEROFAULT
    memset(tempArray+numberElements_,0,
	     (maximumElements-numberElements_)*sizeof(CoinModelTriple)) ;
#   endif
    delete [] elements_;
    elements_=tempArray;
    if (hashElements_.numberItems())
      hashElements_.resize(maximumElements,elements_);
    maximumElements_=maximumElements;
    // If we have links we need to resize
    if ((links_&1)!=0) {
      rowList_.resize(maximumRows_,maximumElements_);
    }
    if ((links_&2)!=0) {
      columnList_.resize(maximumColumns_,maximumElements_);
    }
  }
}
void
CoinModel::fillRows(int whichRow, bool forceCreation,bool fromAddRow)
{
  if (forceCreation||fromAddRow) {
    if (type_==-1) {
      // initial
      type_=0;
      resize(CoinMax(100,whichRow+1),0,1000);
    } else if (type_==1) {
      type_=2;
    }
    if (!rowLower_) {
      // need to set all
      whichRow = numberRows_-1;
      numberRows_=0;
      if (type_!=3)
	resize(CoinMax(100,whichRow+1),0,0);
      else
	resize(CoinMax(1,whichRow+1),0,0);
    }
    if (whichRow>=maximumRows_) {
      if (type_!=3)
      resize(CoinMax((3*maximumRows_)/2,whichRow+1),0,0);
      else
	resize(CoinMax(1,whichRow+1),0,0);
    }
  }
  if (whichRow>=numberRows_&&rowLower_) {
    // Need to fill
    int i;
    for ( i=numberRows_;i<=whichRow;i++) {
      rowLower_[i]=-COIN_DBL_MAX;
      rowUpper_[i]=COIN_DBL_MAX;
      rowType_[i]=0;
    }
  }
  if ( !fromAddRow) {
    numberRows_=CoinMax(whichRow+1,numberRows_);
    // If simple minded then delete start
    if (start_) {
      delete [] start_;
      start_ = NULL;
      assert (!links_);
      // mixed - do linked lists for rows
      createList(1);
    }
  }
}
void
CoinModel::fillColumns(int whichColumn,bool forceCreation,bool fromAddColumn)
{
  if (forceCreation||fromAddColumn) {
    if (type_==-1) {
      // initial
      type_=1;
      resize(0,CoinMax(100,whichColumn+1),1000);
    } else if (type_==0) {
      type_=2;
    }
    if (!objective_) {
      // need to set all
      whichColumn = numberColumns_-1;
      numberColumns_=0;
      if (type_!=3)
	resize(0,CoinMax(100,whichColumn+1),0);
      else
	resize(0,CoinMax(1,whichColumn+1),0);
    }
    if (whichColumn>=maximumColumns_) {
      if (type_!=3)
	resize(0,CoinMax((3*maximumColumns_)/2,whichColumn+1),0);
      else
	resize(0,CoinMax(1,whichColumn+1),0);
    }
  }
  if (whichColumn>=numberColumns_&&objective_) {
    // Need to fill
    int i;
    for ( i=numberColumns_;i<=whichColumn;i++) {
      columnLower_[i]=0.0;
      columnUpper_[i]=COIN_DBL_MAX;
      objective_[i]=0.0;
      integerType_[i]=0;
      columnType_[i]=0;
    }
  }
  if ( !fromAddColumn) {
    numberColumns_=CoinMax(whichColumn+1,numberColumns_);
    // If simple minded then delete start
    if (start_) {
      delete [] start_;
      start_ = NULL;
      assert (!links_);
      // mixed - do linked lists for columns
      createList(2);
    }
  }
}
// Fill in default linked list information
void 
CoinModel::fillList(int which, CoinModelLinkedList & list, int type) const
{
  if ((links_&type)==0) {
    // Create list
    assert (!list.numberMajor());
    if (type==1) {
      list.create(maximumRows_,maximumElements_,numberRows_,numberColumns_,0,
                  numberElements_,elements_);
    } else {
      list.create(maximumColumns_,maximumElements_,numberColumns_,numberRows_,1,
                  numberElements_,elements_);
    }
    if (links_==1 && type== 2) {
      columnList_.synchronize(rowList_);
    } else if (links_==2 && type==1) {
      rowList_.synchronize(columnList_);
    }
    links_ |= type;
  }
  int number = list.numberMajor();
  if (which>=number) {
    // may still need to extend list or fill it in
    if (which>=list.maximumMajor()) {
      list.resize((which*3)/2+100,list.maximumElements());
    }
    list.fill(number,which+1);
  }
}
/* Gets sorted row - user must provide enough space 
   (easiest is allocate number of columns).
   Returns number of elements
*/
int CoinModel::getRow(int whichRow, int * column, double * element)
{
  if (!hashElements_.maximumItems()) {
    // set up number of items
    hashElements_.setNumberItems(numberElements_);
    hashElements_.resize(maximumElements_,elements_);
  }
  assert (whichRow>=0);
  int n=0;
  if (whichRow<numberRows_) {
    CoinModelLink triple=firstInRow(whichRow);
    bool sorted=true;
    int last=-1;
    while (triple.column()>=0) {
      int iColumn = triple.column();
      assert (whichRow==triple.row());
      if (iColumn<last)
        sorted=false;
      last=iColumn;
      if (column)
	column[n]=iColumn;
      if (element)
	element[n]=triple.value();
      n++;
      triple=next(triple);
    }
    if (!sorted) {
      CoinSort_2(column,column+n,element);
    }
  }
  return n;
}
/* Gets sorted column - user must provide enough space 
   (easiest is allocate number of rows).
   Returns number of elements
*/
int CoinModel::getColumn(int whichColumn, int * row, double * element)
{
  if (!hashElements_.maximumItems()) {
    // set up number of items
    hashElements_.setNumberItems(numberElements_);
    hashElements_.resize(maximumElements_,elements_);
  }
  assert (whichColumn>=0);
  int n=0;
  if (whichColumn<numberColumns_) {
    CoinModelLink triple=firstInColumn(whichColumn);
    bool sorted=true;
    int last=-1;
    while (triple.column()>=0) {
      int iRow = triple.row();
      assert (whichColumn==triple.column());
      if (iRow<last)
        sorted=false;
      last=iRow;
      if (row)
	row[n]=iRow;
      if (element)
	element[n]=triple.value();
      n++;
      triple=next(triple);
    }
    if (!sorted) {
      CoinSort_2(row,row+n,element);
    }
  }
  return n;
}
/* Create a linked list and synchronize free 
   type 1 for row 2 for column
   Marked as const as list is mutable */
void 
CoinModel::createList(int type) const
{
  type_=2;
  if (type==1) {
    assert ((links_&1)==0);
    rowList_.create(maximumRows_,maximumElements_,
                    numberRows_,numberColumns_,0,
                    numberElements_,elements_);
    if (links_==2) {
      // synchronize free list
      rowList_.synchronize(columnList_);
    }
    links_ |= 1;
  } else {
    assert ((links_&2)==0);
    columnList_.create(maximumColumns_,maximumElements_,
                       numberColumns_,numberRows_,1,
                       numberElements_,elements_);
    if (links_==1) {
      // synchronize free list
      columnList_.synchronize(rowList_);
    }
    links_ |= 2;
  }
}
// Checks that links are consistent
void 
CoinModel::validateLinks() const
{
  if ((links_&1)) {
    // validate row links
    rowList_.validateLinks(elements_);
  }
  if ((links_&2)) {
    // validate column links
    columnList_.validateLinks(elements_);
  }
}
// returns jColumn (-2 if linear term, -1 if unknown) and coefficient
int 
CoinModel::decodeBit(char * phrase, char * & nextPhrase, double & coefficient, bool ifFirst) const
{
  char * pos = phrase;
  // may be leading - (or +)
  char * pos2 = pos;
  double value=1.0;
  if (*pos2=='-'||*pos2=='+')
    pos2++;
  // next terminator * or + or -
  while (*pos2) {
    if (*pos2=='*') {
      break;
    } else if (*pos2=='-'||*pos2=='+') {
      if (pos2==pos||*(pos2-1)!='e')
	break;
    }
    pos2++;
  }
  // if * must be number otherwise must be name
  if (*pos2=='*') {
    char * pos3 = pos;
    while (pos3!=pos2) {
#ifndef NDEBUG
      char x = *pos3;
#endif
      pos3++;
      assert ((x>='0'&&x<='9')||x=='.'||x=='+'||x=='-'||x=='e');
    }
    char saved = *pos2;
    *pos2='\0';
    value = atof(pos);
    *pos2=saved;
    // and down to next
    pos2++;
    pos=pos2;
    while (*pos2) {
      if (*pos2=='-'||*pos2=='+')
	break;
      pos2++;
    }
  }
  char saved = *pos2;
  *pos2='\0';
  // now name
  // might have + or -
  if (*pos=='+') {
    pos++;
  } else if (*pos=='-') {
    pos++;
    assert (value==1.0);
    value = - value;
  }
  int jColumn = column(pos);
  // must be column unless first when may be linear term
  if (jColumn<0) {
    if (ifFirst) {
      char * pos3 = pos;
      while (pos3!=pos2) {
#ifndef NDEBUG
	char x = *pos3;
#endif
	pos3++;
	assert ((x>='0'&&x<='9')||x=='.'||x=='+'||x=='-'||x=='e');
      }
      assert(*pos2=='\0');
      // keep possible -
      value = value * atof(pos);
      jColumn=-2;
    } else {
      // bad
      *pos2=saved;
      printf("bad nonlinear term %s\n",phrase);
      // maybe return -1 
      abort();
    }
  }
  *pos2=saved;
  pos=pos2;
  coefficient=value;
  nextPhrase = pos;
  return jColumn;
}
/* Gets correct form for a quadratic row - user to delete
   If row is not quadratic then returns which other variables are involved
   with tiny elements and count of total number of variables which could not
   be put in quadratic form
*/
CoinPackedMatrix * 
CoinModel::quadraticRow(int rowNumber,double * linearRow,
			int & numberBad) const
{
  numberBad=0;
  CoinZeroN(linearRow,numberColumns_);
  int numberElements=0;
  assert (rowNumber>=-1&&rowNumber<numberRows_);
  if (rowNumber!=-1) {
    // not objective 
    CoinModelLink triple=firstInRow(rowNumber);
    while (triple.column()>=0) {
      int iColumn = triple.column();
      const char * expr = getElementAsString(rowNumber,iColumn);
      if (strcmp(expr,"Numeric")) {
	// try and see which columns
	assert (strlen(expr)<20000);
	char temp[20000];
	strcpy(temp,expr);
	char * pos = temp;
	bool ifFirst=true;
	while (*pos) {
	  double value;
	  int jColumn = decodeBit(pos, pos, value, ifFirst);
	  // must be column unless first when may be linear term
	  if (jColumn>=0) {
	    numberElements++;
	  } else if (jColumn==-2) {
	    linearRow[iColumn]=value;
	  } else if (jColumn==-1) {
	    // nonlinear term - we will just be marking
	    numberElements++;
	  } else {
	    printf("bad nonlinear term %s\n",temp);
	    abort();
	  }
	  ifFirst=false;
	}
      } else {
	linearRow[iColumn]=getElement(rowNumber,iColumn);
      }
      triple=next(triple);
    }
    if (!numberElements) {
      return NULL;
    } else {
      int * column = new int[numberElements];
      int * column2 = new int[numberElements];
      double * element = new double[numberElements];
      numberElements=0;
      CoinModelLink triple=firstInRow(rowNumber);
      while (triple.column()>=0) {
	int iColumn = triple.column();
	const char * expr = getElementAsString(rowNumber,iColumn);
	if (strcmp(expr,"Numeric")) {
	  // try and see which columns
	  assert (strlen(expr)<20000);
	  char temp[20000];
	  strcpy(temp,expr);
	  char * pos = temp;
	  bool ifFirst=true;
	  while (*pos) {
	    double value;
	    int jColumn = decodeBit(pos, pos, value, ifFirst);
	    // must be column unless first when may be linear term
	    if (jColumn>=0) {
	      column[numberElements]=iColumn;
	      column2[numberElements]=jColumn;
	      element[numberElements++]=value;
	    } else if (jColumn==-1) {
	      // nonlinear term - we will just be marking
	      assert (jColumn>=0);
	      column[numberElements]=iColumn;
	      column2[numberElements]=jColumn;
	      element[numberElements++]=1.0e-100;
	      numberBad++;
	    } else if (jColumn!=-2) {
	      printf("bad nonlinear term %s\n",temp);
	      abort();
	    }
	    ifFirst=false;
	  }
	}
	triple=next(triple);
      }
      CoinPackedMatrix * newMatrix = new CoinPackedMatrix(true,column2,column,element,numberElements);
      delete [] column;
      delete [] column2;
      delete [] element;
      return newMatrix;
    }
  } else {
    // objective
    int iColumn;
    for (iColumn=0;iColumn<numberColumns_;iColumn++) {
      const char * expr = getColumnObjectiveAsString(iColumn);
      if (strcmp(expr,"Numeric")) {
	// try and see which columns
	assert (strlen(expr)<20000);
	char temp[20000];
	strcpy(temp,expr);
	char * pos = temp;
	bool ifFirst=true;
	while (*pos) {
	  double value;
	  int jColumn = decodeBit(pos, pos, value, ifFirst);
	  // must be column unless first when may be linear term
	  if (jColumn>=0) {
	    numberElements++;
	  } else if (jColumn==-2) {
	    linearRow[iColumn]=value;
	  } else if (jColumn==-1) {
	    // nonlinear term - we will just be marking
	    numberElements++;
	  } else {
	    printf("bad nonlinear term %s\n",temp);
	    abort();
	  }
	  ifFirst=false;
	}
      } else {
	linearRow[iColumn]=getElement(rowNumber,iColumn);
      }
    }
    if (!numberElements) {
      return NULL;
    } else {
      int * column = new int[numberElements];
      int * column2 = new int[numberElements];
      double * element = new double[numberElements];
      numberElements=0;
      for (iColumn=0;iColumn<numberColumns_;iColumn++) {
	const char * expr = getColumnObjectiveAsString(iColumn);
	if (strcmp(expr,"Numeric")) {
	  // try and see which columns
	  assert (strlen(expr)<20000);
	  char temp[20000];
	  strcpy(temp,expr);
	  char * pos = temp;
	  bool ifFirst=true;
	  while (*pos) {
	    double value;
	    int jColumn = decodeBit(pos, pos, value, ifFirst);
	    // must be column unless first when may be linear term
	    if (jColumn>=0) {
	      column[numberElements]=iColumn;
	      column2[numberElements]=jColumn;
	      element[numberElements++]=value;
	    } else if (jColumn==-1) {
	      // nonlinear term - we will just be marking
	      assert (jColumn>=0);
	      column[numberElements]=iColumn;
	      column2[numberElements]=jColumn;
	      element[numberElements++]=1.0e-100;
	      numberBad++;
	    } else if (jColumn!=-2) {
	      printf("bad nonlinear term %s\n",temp);
	      abort();
	    }
	    ifFirst=false;
	  }
	}
      }
      return new CoinPackedMatrix(true,column2,column,element,numberElements);
    }
  }
}
// Replaces a quadratic row
void 
CoinModel::replaceQuadraticRow(int rowNumber,const double * linearRow, const CoinPackedMatrix * quadraticPart)
{
  assert (rowNumber>=-1&&rowNumber<numberRows_);
  if (rowNumber>=0) {
    CoinModelLink triple=firstInRow(rowNumber);
    while (triple.column()>=0) {
      int iColumn = triple.column();
      deleteElement(rowNumber,iColumn);
      // triple stale - so start over
      triple=firstInRow(rowNumber);
    }
    const double * element = quadraticPart->getElements();
    const int * column = quadraticPart->getIndices();
    const CoinBigIndex * columnStart = quadraticPart->getVectorStarts();
    const int * columnLength = quadraticPart->getVectorLengths();
    int numberLook = quadraticPart->getNumCols();
    int i;
    for (i=0;i<numberLook;i++) {
      if (!columnLength[i]) {
	// just linear part
	if (linearRow[i])
	  setElement(rowNumber,i,linearRow[i]);
      } else {
	char temp[10000];
	int put=0;
	char temp2[30];
	bool first=true;
	if (linearRow[i]) {
	  sprintf(temp,"%g",linearRow[i]);
	  first=false;
          /* temp is at most 10000 long, so static_cast is safe */
	  put = static_cast<int>(strlen(temp));
	}
	for (int j=columnStart[i];j<columnStart[i]+columnLength[i];j++) {
	  int jColumn = column[j];
	  double value = element[j];
	  if (value<0.0||first) 
	    sprintf(temp2,"%g*c%7.7d",value,jColumn);
	  else
	    sprintf(temp2,"+%g*c%7.7d",value,jColumn);
	  int nextPut = put + static_cast<int>(strlen(temp2));
	  assert (nextPut<10000);
	  strcpy(temp+put,temp2);
	  put = nextPut;
	}
	setElement(rowNumber,i,temp);
      }
    }
    // rest of linear
    for (;i<numberColumns_;i++) {
      if (linearRow[i])
	setElement(rowNumber,i,linearRow[i]);
    }
  } else {
    // objective
    int iColumn;
    for (iColumn=0;iColumn<numberColumns_;iColumn++) {
      setColumnObjective(iColumn,0.0);
    }
    const double * element = quadraticPart->getElements();
    const int * column = quadraticPart->getIndices();
    const CoinBigIndex * columnStart = quadraticPart->getVectorStarts();
    const int * columnLength = quadraticPart->getVectorLengths();
    int numberLook = quadraticPart->getNumCols();
    int i;
    for (i=0;i<numberLook;i++) {
      if (!columnLength[i]) {
	// just linear part
	if (linearRow[i])
	  setColumnObjective(i,linearRow[i]);
      } else {
	char temp[10000];
	int put=0;
	char temp2[30];
	bool first=true;
	if (linearRow[i]) {
	  sprintf(temp,"%g",linearRow[i]);
	  first=false;
          /* temp is at most 10000 long, so static_cast is safe */
	  put = static_cast<int>(strlen(temp));
	}
	for (int j=columnStart[i];j<columnStart[i]+columnLength[i];j++) {
	  int jColumn = column[j];
	  double value = element[j];
	  if (value<0.0||first) 
	    sprintf(temp2,"%g*c%7.7d",value,jColumn);
	  else
	    sprintf(temp2,"+%g*c%7.7d",value,jColumn);
	  int nextPut = put + static_cast<int>(strlen(temp2));
	  assert (nextPut<10000);
	  strcpy(temp+put,temp2);
	  put = nextPut;
	}
	setColumnObjective(i,temp);
      }
    }
    // rest of linear
    for (;i<numberColumns_;i++) {
      if (linearRow[i])
	setColumnObjective(i,linearRow[i]);
    }
  }
}
/* If possible return a model where if all variables marked nonzero are fixed
      the problem will be linear.  At present may only work if quadratic.
      Returns NULL if not possible
*/
CoinModel * 
CoinModel::reorder(const char * mark) const
{
  // redo array so 2 high priority nonlinear, 1 nonlinear, 0 linear
  char * highPriority = new char [numberColumns_];
  double * linear = new double[numberColumns_];
  CoinModel * newModel = new CoinModel(*this);
  int iRow;
  for (iRow=-1;iRow<numberRows_;iRow++) {
    int numberBad;
    CoinPackedMatrix * row = quadraticRow(iRow,linear,numberBad);
    assert (!numberBad); // fix later
    if (row) {
      // see if valid
      //const double * element = row->getElements();
      const int * column = row->getIndices();
      const CoinBigIndex * columnStart = row->getVectorStarts();
      const int * columnLength = row->getVectorLengths();
      int numberLook = row->getNumCols();
      for (int i=0;i<numberLook;i++) {
	if (mark[i])
	  highPriority[i]=2;
	else
	  highPriority[i]=1;
	for (int j=columnStart[i];j<columnStart[i]+columnLength[i];j++) {
	  int iColumn = column[j];
	  if (mark[iColumn])
	    highPriority[iColumn]=2;
	  else
	    highPriority[iColumn]=1;
	}
      }
      delete row;
    }
  }
  for (iRow=-1;iRow<numberRows_;iRow++) {
    int numberBad;
    CoinPackedMatrix * row = quadraticRow(iRow,linear,numberBad);
    if (row) {
      // see if valid
      const double * element = row->getElements();
      const int * columnLow = row->getIndices();
      const CoinBigIndex * columnHigh = row->getVectorStarts();
      const int * columnLength = row->getVectorLengths();
      int numberLook = row->getNumCols();
      int canSwap=0;
      for (int i=0;i<numberLook;i++) {
	// this one needs to be available
	int iPriority = highPriority[i];
	for (int j=columnHigh[i];j<columnHigh[i]+columnLength[i];j++) {
	  int iColumn = columnLow[j];
	  if (highPriority[iColumn]<=1) {
	    assert (highPriority[iColumn]==1);
	    if (iPriority==1) {
	      canSwap=-1; // no good
	      break;
	    } else {
	      canSwap=1;
	    }
	  }
	}
      }
      if (canSwap) {
	if (canSwap>0) {
	  // rewrite row
	  /* get triples
	     then swap ones needed
	     then create packedmatrix
	     then replace row
	  */
	  int numberElements=columnHigh[numberLook];
	  int * columnHigh2 = new int [numberElements];
	  int * columnLow2 = new int [numberElements];
	  double * element2 = new double [numberElements];
	  for (int i=0;i<numberLook;i++) {
	    // this one needs to be available
	    int iPriority = highPriority[i];
	    if (iPriority==2) {
	      for (int j=columnHigh[i];j<columnHigh[i]+columnLength[i];j++) {
		columnHigh2[j]=i;
		columnLow2[j]=columnLow[j];
		element2[j]=element[j];
	      }
	    } else {
	      for (int j=columnHigh[i];j<columnHigh[i]+columnLength[i];j++) {
		columnLow2[j]=i;
		columnHigh2[j]=columnLow[j];
		element2[j]=element[j];
	      }
	    }
	  }
	  delete row;
	  row = new CoinPackedMatrix(true,columnHigh2,columnLow2,element2,numberElements);
	  delete [] columnHigh2;
	  delete [] columnLow2;
	  delete [] element2;
	  // Now replace row
	  newModel->replaceQuadraticRow(iRow,linear,row);
	  delete row;
	} else {
	  delete row;
	  delete newModel;
	  newModel=NULL;
	  printf("Unable to use priority - row %d\n",iRow);
	  break;
	}
      }
    }
  }
  delete [] highPriority;
  delete [] linear;
  return newModel;
}
// Sets cut marker array
void 
CoinModel::setCutMarker(int size,const int * marker)
{
  delete [] cut_;
  cut_ = new int [maximumRows_];
  CoinZeroN(cut_,maximumRows_);
  CoinMemcpyN(marker,size,cut_);
}
// Sets priority array
void 
CoinModel::setPriorities(int size,const int * priorities)
{
  delete [] priority_;
  priority_ = new int [maximumColumns_];
  CoinZeroN(priority_,maximumColumns_);
  CoinMemcpyN(priorities,size,priority_);
}
/* Sets columnObjective array
 */
void 
CoinModel::setObjective(int numberColumns,const double * objective) 
{
  fillColumns(numberColumns,true,true);
  for (int i=0;i<numberColumns;i++) {
    objective_[i]=objective[i];
    columnType_[i] &= ~4;
  }
}
/* Sets columnLower array
 */
void 
CoinModel::setColumnLower(int numberColumns,const double * columnLower)
{
  fillColumns(numberColumns,true,true);
  for (int i=0;i<numberColumns;i++) {
    columnLower_[i]=columnLower[i];
    columnType_[i] &= ~1;
  }
}
/* Sets columnUpper array
 */
void 
CoinModel::setColumnUpper(int numberColumns,const double * columnUpper)
{
  fillColumns(numberColumns,true,true);
  for (int i=0;i<numberColumns;i++) {
    columnUpper_[i]=columnUpper[i];
    columnType_[i] &= ~2;
  }
}
/* Sets rowLower array
 */
void 
CoinModel::setRowLower(int numberRows,const double * rowLower)
{
  fillColumns(numberRows,true,true);
  for (int i=0;i<numberRows;i++) {
    rowLower_[i]=rowLower[i];
    rowType_[i] &= ~1;
  }
}
/* Sets rowUpper array
 */
void 
CoinModel::setRowUpper(int numberRows,const double * rowUpper)
{
  fillColumns(numberRows,true,true);
  for (int i=0;i<numberRows;i++) {
    rowUpper_[i]=rowUpper[i];
    rowType_[i] &= ~2;
  }
}
// Pass in CoinPackedMatrix (and switch off element updates)
void 
CoinModel::passInMatrix(const CoinPackedMatrix & matrix)
{
  type_=3;
  packedMatrix_ = new CoinPackedMatrix(matrix);
}
// Convert elements to CoinPackedMatrix (and switch off element updates)
int 
CoinModel::convertMatrix()
{
  int numberErrors=0;
  if (type_!=3) {
    // If strings then do copies
    if (string_.numberItems()) {
      numberErrors = createArrays(rowLower_, rowUpper_, 
				  columnLower_, columnUpper_,
				  objective_, integerType_,associated_);
    }
    CoinPackedMatrix matrix;
    createPackedMatrix(matrix,associated_);
    packedMatrix_ = new CoinPackedMatrix(matrix);
    type_=3;
  }
  return numberErrors;
}
// Aborts with message about packedMatrix
void 
CoinModel::badType() const
{
  fprintf(stderr,"******** operation not allowed when in block mode ****\n");
  abort();
}

//#############################################################################
// Methods to input a problem
//#############################################################################
/** A function to convert from the lb/ub style of constraint
    definition to the sense/rhs/range style */
void
convertBoundToSense(const double lower, const double upper,
					char& sense, double& right,
					double& range) 
{
  double inf = 1.0e-30;
  range = 0.0;
  if (lower > -inf) {
    if (upper < inf) {
      right = upper;
      if (upper==lower) {
        sense = 'E';
      } else {
        sense = 'R';
        range = upper - lower;
      }
    } else {
      sense = 'G';
      right = lower;
    }
  } else {
    if (upper < inf) {
      sense = 'L';
      right = upper;
    } else {
      sense = 'N';
      right = 0.0;
    }
  }
}

//-----------------------------------------------------------------------------
/** A function to convert from the sense/rhs/range style of
    constraint definition to the lb/ub style */
void
convertSenseToBound(const char sense, const double right,
					const double range,
					double& lower, double& upper) 
{
  double inf=COIN_DBL_MAX;
  switch (sense) {
  case 'E':
    lower = upper = right;
    break;
  case 'L':
    lower = -inf;
    upper = right;
    break;
  case 'G':
    lower = right;
    upper = inf;
    break;
  case 'R':
    lower = right - range;
    upper = right;
    break;
  case 'N':
    lower = -inf;
    upper = inf;
    break;
  }
}

void
CoinModel::loadBlock(const CoinPackedMatrix& matrix,
				   const double* collb, const double* colub,   
				   const double* obj,
				   const double* rowlb, const double* rowub)
{
  passInMatrix(matrix);
  int numberRows=matrix.getNumRows();
  int numberColumns=matrix.getNumCols();
  setObjective(numberColumns,obj);
  setRowLower(numberRows,rowlb);
  setRowUpper(numberRows,rowub);
  setColumnLower(numberColumns,collb);
  setColumnUpper(numberColumns,colub);
}

//-----------------------------------------------------------------------------

void
CoinModel::loadBlock(const CoinPackedMatrix& matrix,
				   const double* collb, const double* colub,
				   const double* obj,
				   const char* rowsen, const double* rowrhs,   
				   const double* rowrng)
{
  // If any of Rhs NULLs then create arrays
  int numrows = matrix.getNumRows();
  const char * rowsenUse = rowsen;
  if (!rowsen) {
    char * rowsen = new char [numrows];
    for (int i=0;i<numrows;i++)
      rowsen[i]='G';
    rowsenUse = rowsen;
  } 
  const double * rowrhsUse = rowrhs;
  if (!rowrhs) {
    double * rowrhs = new double [numrows];
    for (int i=0;i<numrows;i++)
      rowrhs[i]=0.0;
    rowrhsUse = rowrhs;
  }
  const double * rowrngUse = rowrng;
  if (!rowrng) {
    double * rowrng = new double [numrows];
    for (int i=0;i<numrows;i++)
      rowrng[i]=0.0;
    rowrngUse = rowrng;
  }
  double * rowlb = new double[numrows];
  double * rowub = new double[numrows];
  for (int i = numrows-1; i >= 0; --i) {   
    convertSenseToBound(rowsenUse[i],rowrhsUse[i],rowrngUse[i],rowlb[i],rowub[i]);
  }
  if (rowsen!=rowsenUse)
    delete [] rowsenUse;
  if (rowrhs!=rowrhsUse)
    delete [] rowrhsUse;
  if (rowrng!=rowrngUse)
    delete [] rowrngUse;
  loadBlock(matrix, collb, colub, obj, rowlb, rowub);
  delete [] rowlb;
  delete [] rowub;
}

//-----------------------------------------------------------------------------

void
CoinModel::loadBlock(const int numcols, const int numrows,
		     const CoinBigIndex * start, const int* index,
		     const double* value,
		     const double* collb, const double* colub,
		     const double* obj,
		     const double* rowlb, const double* rowub)
{
  int numberElements = start[numcols];
  int * length = new int [numcols];
  for (int i=0;i<numcols;i++) 
    length[i]=start[i+1]-start[i];
  CoinPackedMatrix matrix(true,numrows,numcols,numberElements,value,
			  index,start,length,0.0,0.0);
  loadBlock(matrix, collb, colub, obj, rowlb, rowub);
  delete [] length;
}
//-----------------------------------------------------------------------------

void
CoinModel::loadBlock(const int numcols, const int numrows,
		     const CoinBigIndex * start, const int* index,
		     const double* value,
		     const double* collb, const double* colub,
		     const double* obj,
		     const char* rowsen, const double* rowrhs,   
		     const double* rowrng)
{
  // If any of Rhs NULLs then create arrays
  const char * rowsenUse = rowsen;
  if (!rowsen) {
    char * rowsen = new char [numrows];
    for (int i=0;i<numrows;i++)
      rowsen[i]='G';
    rowsenUse = rowsen;
  } 
  const double * rowrhsUse = rowrhs;
  if (!rowrhs) {
    double * rowrhs = new double [numrows];
    for (int i=0;i<numrows;i++)
      rowrhs[i]=0.0;
    rowrhsUse = rowrhs;
  }
  const double * rowrngUse = rowrng;
  if (!rowrng) {
    double * rowrng = new double [numrows];
    for (int i=0;i<numrows;i++)
      rowrng[i]=0.0;
    rowrngUse = rowrng;
  }
  double * rowlb = new double[numrows];
  double * rowub = new double[numrows];
  for (int i = numrows-1; i >= 0; --i) {   
    convertSenseToBound(rowsenUse[i],rowrhsUse[i],rowrngUse[i],rowlb[i],rowub[i]);
  }
  if (rowsen!=rowsenUse)
    delete [] rowsenUse;
  if (rowrhs!=rowrhsUse)
    delete [] rowrhsUse;
  if (rowrng!=rowrngUse)
    delete [] rowrngUse;
  int numberElements = start[numcols];
  int * length = new int [numcols];
  for (int i=0;i<numcols;i++) 
    length[i]=start[i+1]-start[i];
  CoinPackedMatrix matrix(true,numrows,numcols,numberElements,value,
			  index,start,length,0.0,0.0);
  loadBlock(matrix, collb, colub, obj, rowlb, rowub);
  delete [] length;
  delete[] rowlb;
  delete[] rowub;
}
/* Returns which parts of model are set
   1 - matrix
   2 - rhs
   4 - row names
   8 - column bounds and/or objective
   16 - column names
   32 - integer types
*/
int 
CoinModel::whatIsSet() const
{
  int type = (numberElements_) ? 1 : 0;
  bool defaultValues=true;
  if (rowLower_) {
    for (int i=0;i<numberRows_;i++) {
      if (rowLower_[i]!=-COIN_DBL_MAX) {
	defaultValues=false;
	break;
      }
      if (rowUpper_[i]!=COIN_DBL_MAX) {
	defaultValues=false;
	break;
      }
    }
  }
  if (!defaultValues)
    type |= 2;
  if (rowName_.numberItems())
    type |= 4;
  defaultValues=true;
  if (columnLower_) {
    for (int i=0;i<numberColumns_;i++) {
      if (objective_[i]!=0.0) {
	defaultValues=false;
	break;
      }
      if (columnLower_[i]!=0.0) {
	defaultValues=false;
	break;
      }
      if (columnUpper_[i]!=COIN_DBL_MAX) {
	defaultValues=false;
	break;
      }
    }
  }
  if (!defaultValues)
    type |= 8;
  if (columnName_.numberItems())
    type |= 16;
  defaultValues=true;
  if (integerType_) {
    for (int i=0;i<numberColumns_;i++) {
      if (integerType_[i]) {
	defaultValues=false;
	break;
      }
    }
  }
  if (!defaultValues)
    type |= 32;
  return type;
}
// For decomposition set original row and column indices
void 
CoinModel::setOriginalIndices(const int * row, const int * column)
{
  if (!rowType_)
    rowType_ = new int [numberRows_];
  memcpy(rowType_,row,numberRows_*sizeof(int));
  if (!columnType_)
    columnType_ = new int [numberColumns_];
  memcpy(columnType_,column,numberColumns_*sizeof(int));
}
