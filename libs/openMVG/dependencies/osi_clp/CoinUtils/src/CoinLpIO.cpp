/* $Id: CoinLpIO.cpp 1602 2013-07-12 12:15:37Z tkr $ */
// Last edit: 11/5/08
//
// Name:     CoinLpIO.cpp; Support for Lp files
// Author:   Francois Margot
//           Tepper School of Business
//           Carnegie Mellon University, Pittsburgh, PA 15213
// Date:     12/28/03
//-----------------------------------------------------------------------------
// Copyright (C) 2003, Francois Margot, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinUtilsConfig.h"

#include <cmath>
#include <cfloat>
#include <cctype>
#include <cassert>
#include <string>
#include <cstdarg>

#include "CoinError.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinLpIO.hpp"
#include "CoinFinite.hpp"

using namespace std;

//#define LPIO_DEBUG

/************************************************************************/

CoinLpIO::CoinLpIO() :
  problemName_(CoinStrdup("")),
  defaultHandler_(true),
  numberRows_(0),
  numberColumns_(0),
  numberElements_(0),
  matrixByColumn_(NULL),
  matrixByRow_(NULL),
  rowlower_(NULL),
  rowupper_(NULL),
  collower_(NULL),
  colupper_(NULL),
  rhs_(NULL),
  rowrange_(NULL),
  rowsense_(NULL),
  objective_(NULL),
  objectiveOffset_(0),
  integerType_(NULL),
  fileName_(NULL),
  infinity_(COIN_DBL_MAX),
  epsilon_(1e-5),
  numberAcross_(10),
  decimals_(5),
  objName_(NULL)
{
  card_previous_names_[0] = 0;
  card_previous_names_[1] = 0;
  previous_names_[0] = NULL;
  previous_names_[1] = NULL;

  maxHash_[0]=0;
  numberHash_[0]=0;
  hash_[0] = NULL;
  names_[0] = NULL;
  maxHash_[1] = 0;
  numberHash_[1] = 0;
  hash_[1] = NULL;
  names_[1] = NULL;
  handler_ = new CoinMessageHandler();
  messages_ = CoinMessage();
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
CoinLpIO::CoinLpIO(const CoinLpIO& rhs)
    :
    problemName_(CoinStrdup("")),
    defaultHandler_(true),
    numberRows_(0),
    numberColumns_(0),
    numberElements_(0),
    rowsense_(NULL),
    rhs_(NULL),
    rowrange_(NULL),
    matrixByRow_(NULL),
    matrixByColumn_(NULL),
    rowlower_(NULL),
    rowupper_(NULL),
    collower_(NULL),
    colupper_(NULL),
    objective_(NULL),
    objectiveOffset_(0.0),
    integerType_(NULL),
    fileName_(CoinStrdup("")),
    infinity_(COIN_DBL_MAX),
    numberAcross_(10),
    epsilon_(1e-5),
    objName_(NULL)
{
    card_previous_names_[0] = 0;
    card_previous_names_[1] = 0;
    previous_names_[0] = NULL;
    previous_names_[1] = NULL;
    maxHash_[0] = 0;
    numberHash_[0] = 0;
    hash_[0] = NULL;
    names_[0] = NULL;
    maxHash_[1] = 0;
    numberHash_[1] = 0;
    hash_[1] = NULL;
    names_[1] = NULL;
 
    if ( rhs.rowlower_ != NULL || rhs.collower_ != NULL) {
       gutsOfCopy(rhs);
    }
 
    defaultHandler_ = rhs.defaultHandler_;
 
    if (defaultHandler_) {
       handler_ = new CoinMessageHandler(*rhs.handler_);
    } else {
       handler_ = rhs.handler_;
    }
 
    messages_ = CoinMessage();
}


void CoinLpIO::gutsOfCopy(const CoinLpIO& rhs)
{
    defaultHandler_ = rhs.defaultHandler_;
 
    if (rhs.matrixByRow_) {
       matrixByRow_ = new CoinPackedMatrix(*(rhs.matrixByRow_));
    }
 
    numberElements_ = rhs.numberElements_;
    numberRows_ = rhs.numberRows_;
    numberColumns_ = rhs.numberColumns_;
    decimals_ = rhs.decimals_;
 
    if (rhs.rowlower_) {
       rowlower_ = reinterpret_cast<double*> (malloc(numberRows_ * sizeof(double)));
       rowupper_ = reinterpret_cast<double*> (malloc(numberRows_ * sizeof(double)));
       memcpy(rowlower_, rhs.rowlower_, numberRows_ * sizeof(double));
       memcpy(rowupper_, rhs.rowupper_, numberRows_ * sizeof(double));
       rowrange_ = reinterpret_cast<double *> (malloc(numberRows_*sizeof(double)));
       rowsense_ = reinterpret_cast<char *> (malloc(numberRows_*sizeof(char)));
       rhs_ = reinterpret_cast<double *> (malloc(numberRows_*sizeof(double)));
       memcpy(rowrange_,rhs.getRowRange(),numberRows_*sizeof(double));
       memcpy(rowsense_,rhs.getRowSense(),numberRows_*sizeof(char));
       memcpy(rhs_,rhs.getRightHandSide(),numberRows_*sizeof(double));
    }
 
    if (rhs.collower_) {
       collower_ = reinterpret_cast<double*> (malloc(numberColumns_ * sizeof(double)));
       colupper_ = reinterpret_cast<double*> (malloc(numberColumns_ * sizeof(double)));
       objective_ = reinterpret_cast<double*> (malloc(numberColumns_ * sizeof(double)));
       memcpy(collower_, rhs.collower_, numberColumns_ * sizeof(double));
       memcpy(colupper_, rhs.colupper_, numberColumns_ * sizeof(double));
       memcpy(objective_, rhs.objective_, numberColumns_ * sizeof(double));
    }
 
    if (rhs.integerType_) {
       integerType_ = reinterpret_cast<char*> (malloc (numberColumns_ * sizeof(char)));
       memcpy(integerType_, rhs.integerType_, numberColumns_ * sizeof(char));
    }
 
    free(fileName_);
    free(problemName_);
    fileName_ = CoinStrdup(rhs.fileName_);
    problemName_ = CoinStrdup(rhs.problemName_);
    numberHash_[0] = rhs.numberHash_[0];
    numberHash_[1] = rhs.numberHash_[1];
    maxHash_[0] = rhs.maxHash_[0];
    maxHash_[1] = rhs.maxHash_[1];
    infinity_ = rhs.infinity_;
    numberAcross_ = rhs.numberAcross_;
    objectiveOffset_ = rhs.objectiveOffset_;
    int section;
 
    for (section = 0; section < 2; section++) {
       if (numberHash_[section]) {
          char** names2 = rhs.names_[section];
          names_[section] = reinterpret_cast<char**> (malloc(maxHash_[section] *
                            sizeof(char*)));
          char** names = names_[section];
          int i;
 
          for (i = 0; i < numberHash_[section]; i++) {
             names[i] = CoinStrdup(names2[i]);
          }
 
          hash_[section] = new CoinHashLink[maxHash_[section]];
          std::memcpy(hash_[section], rhs.hash_[section], maxHash_[section]*sizeof(CoinHashLink));
       }
    }
 }

CoinLpIO &
CoinLpIO::operator=(const CoinLpIO& rhs)
{
    if (this != &rhs) {
       gutsOfDestructor();
 
       if ( rhs.rowlower_ != NULL || rhs.collower_ != NULL) {
          gutsOfCopy(rhs);
       }
 
       defaultHandler_ = rhs.defaultHandler_;
 
       if (defaultHandler_) {
          handler_ = new CoinMessageHandler(*rhs.handler_);
       } else {
          handler_ = rhs.handler_;
       }
 
       messages_ = CoinMessage();
    }
 
    return *this;
}


void CoinLpIO::gutsOfDestructor()
 {
    freeAll();
 
    if (defaultHandler_) {
       delete handler_;
       handler_ = NULL;
    }
}



/************************************************************************/
CoinLpIO::~CoinLpIO() {
  stopHash(0);
  stopHash(1);
  freeAll();
  if (defaultHandler_) {
    delete handler_;
    handler_ = NULL; 
  }
}

/************************************************************************/
void
CoinLpIO::freePreviousNames(const int section) {

  int j;

  if(previous_names_[section] != NULL) {
    for(j=0; j<card_previous_names_[section]; j++) {
      free(previous_names_[section][j]);
    }
    free(previous_names_[section]);
  }
  previous_names_[section] = NULL;
  card_previous_names_[section] = 0;
} /* freePreviousNames */

/************************************************************************/
void
CoinLpIO::freeAll() {

  delete matrixByColumn_;
  matrixByColumn_ = NULL; 
  delete matrixByRow_;
  matrixByRow_ = NULL; 
  free(rowupper_);
  rowupper_ = NULL;
  free(rowlower_);
  rowlower_ = NULL;
  free(colupper_);
  colupper_ = NULL;
  free(collower_);
  collower_ = NULL;
  free(rhs_);
  rhs_ = NULL;
  free(rowrange_);
  rowrange_ = NULL;
  free(rowsense_);
  rowsense_ = NULL;
  free(objective_);
  objective_ = NULL;
  free(integerType_);
  integerType_ = NULL;
  free(problemName_);
  problemName_ = NULL;
  free(fileName_);
  fileName_ = NULL;

  freePreviousNames(0);
  freePreviousNames(1);
}

/*************************************************************************/
const char * CoinLpIO::getProblemName() const
{
  return problemName_;
}

void
CoinLpIO::setProblemName (const char *name)
{
  free(problemName_) ;
  problemName_ = CoinStrdup(name);
}

/*************************************************************************/
int CoinLpIO::getNumCols() const
{
  return numberColumns_;
}

/*************************************************************************/
int CoinLpIO::getNumRows() const
{
  return numberRows_;
}

/*************************************************************************/
int CoinLpIO::getNumElements() const
{
  return numberElements_;
}

/*************************************************************************/
const double * CoinLpIO::getColLower() const
{
  return collower_;
}

/*************************************************************************/
const double * CoinLpIO::getColUpper() const
{
  return colupper_;
}

/*************************************************************************/
const double * CoinLpIO::getRowLower() const
{
  return rowlower_;
}

/*************************************************************************/
const double * CoinLpIO::getRowUpper() const
{
  return rowupper_;
}

/*************************************************************************/
/** A quick inlined function to convert from lb/ub style constraint
    definition to sense/rhs/range style */
inline void
CoinLpIO::convertBoundToSense(const double lower, const double upper,
			      char& sense, double& right,
			      double& range) const
{
  range = 0.0;
  if (lower > -infinity_) {
    if (upper < infinity_) {
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
    if (upper < infinity_) {
      sense = 'L';
      right = upper;
    } else {
      sense = 'N';
      right = 0.0;
    }
  }
}

/*************************************************************************/
 const char * CoinLpIO::getRowSense() const
{
  if(rowsense_  == NULL) {
    int nr=numberRows_;
    rowsense_ = reinterpret_cast<char *> (malloc(nr*sizeof(char)));
    
    double dum1,dum2;
    int i;
    for(i=0; i<nr; i++) {
      convertBoundToSense(rowlower_[i],rowupper_[i],rowsense_[i],dum1,dum2);
    }
  }
  return rowsense_;
}

/*************************************************************************/
const double * CoinLpIO::getRightHandSide() const
{
  if(rhs_==NULL) {
    int nr=numberRows_;
    rhs_ = reinterpret_cast<double *> (malloc(nr*sizeof(double)));

    char dum1;
    double dum2;
    int i;
    for (i=0; i<nr; i++) {
      convertBoundToSense(rowlower_[i],rowupper_[i],dum1,rhs_[i],dum2);
    }
  }
  return rhs_;
}

/*************************************************************************/
const double * CoinLpIO::getRowRange() const
{
  if (rowrange_ == NULL) {
    int nr=numberRows_;
    rowrange_ = reinterpret_cast<double *> (malloc(nr*sizeof(double)));
    std::fill(rowrange_,rowrange_+nr,0.0);

    char dum1;
    double dum2;
    int i;
    for (i=0; i<nr; i++) {
      convertBoundToSense(rowlower_[i],rowupper_[i],dum1,dum2,rowrange_[i]);
    }
  }
  return rowrange_;
}

/*************************************************************************/
const double * CoinLpIO::getObjCoefficients() const
{
  return objective_;
}
 
/*************************************************************************/
const CoinPackedMatrix * CoinLpIO::getMatrixByRow() const
{
  return matrixByRow_;
}

/*************************************************************************/
const CoinPackedMatrix * CoinLpIO::getMatrixByCol() const
{
  if (matrixByColumn_ == NULL && matrixByRow_) {
    matrixByColumn_ = new CoinPackedMatrix(*matrixByRow_);
    matrixByColumn_->reverseOrdering();
  }
  return matrixByColumn_;
}

/*************************************************************************/
const char * CoinLpIO::getObjName() const
{
  return objName_;
}
 
/*************************************************************************/
void CoinLpIO::checkRowNames() {

  int i, nrow = getNumRows();

  if(numberHash_[0] != nrow+1) {
    setDefaultRowNames();
    handler_->message(COIN_GENERAL_WARNING,messages_)<<
      "### CoinLpIO::checkRowNames(): non distinct or missing row names or objective function name.\nNow using default row names."
						     <<CoinMessageEol;
  }

  char const * const * rowNames = getRowNames();
  const char *rSense = getRowSense();
  char rName[256];

  // Check that row names and objective function name are all distinct, 
  /// even after adding "_low" to ranged constraint names

  for(i=0; i<nrow; i++) {
    if(rSense[i] == 'R') {
      sprintf(rName, "%s_low", rowNames[i]);
      if(findHash(rName, 0) != -1) {
	setDefaultRowNames();
	char printBuffer[512];
	sprintf(printBuffer,"### CoinLpIO::checkRowNames(): ranged constraint %d hasa name %s identical to another constraint name or objective function name.\nUse getPreviousNames() to get the old row names.\nNow using default row names.", i, rName);
	handler_->message(COIN_GENERAL_WARNING,messages_)<<printBuffer
						  <<CoinMessageEol;
	break;
      }
    }
  }
} /* checkRowNames */

/*************************************************************************/
void CoinLpIO::checkColNames() {

  int ncol = getNumCols();

  if(numberHash_[1] != ncol) {
    setDefaultColNames();
    handler_->message(COIN_GENERAL_WARNING,messages_)<<
      "### CoinLpIO::checkColNames(): non distinct or missing column names.\nNow using default column names."
					      <<CoinMessageEol;
  }
} /* checkColNames */

/*************************************************************************/
void CoinLpIO::getPreviousRowNames(char const * const * prev, 
				   int *card_prev) const
{
  *card_prev = card_previous_names_[0];
  prev = previous_names_[0];
}
 
/*************************************************************************/
void CoinLpIO::getPreviousColNames(char const * const * prev, 
				   int *card_prev) const
{
  *card_prev = card_previous_names_[1];
  prev = previous_names_[1];
}
 
/*************************************************************************/
char const * const * CoinLpIO::getRowNames() const
{
  return names_[0];
}
 
/*************************************************************************/
char const * const * CoinLpIO::getColNames() const
{
  return names_[1];
}
 
/*************************************************************************/
const char * CoinLpIO::rowName(int index) const {

  if((names_[0] != NULL) && (index >= 0) && (index < numberRows_+1)) {
    return names_[0][index];
  } 
  else {
    return NULL;
  }
}

/*************************************************************************/
const char * CoinLpIO::columnName(int index) const {

  if((names_[1] != NULL) && (index >= 0) && (index < numberColumns_)) {
    return names_[1][index];
  } 
  else {
    return NULL;
  }
}

/*************************************************************************/
int CoinLpIO::rowIndex(const char * name) const {

  if (!hash_[0]) {
    return -1;
  }
  return findHash(name , 0);
}

/*************************************************************************/
int CoinLpIO::columnIndex(const char * name) const {

  if (!hash_[1]) {
    return -1;
  }
  return findHash(name , 1);
}

/************************************************************************/
double CoinLpIO::getInfinity() const
{
  return infinity_;
}

/************************************************************************/
void CoinLpIO::setInfinity(const double value) 
{
  if (value >= 1.0e20) {
    infinity_ = value;
  } 
  else {
    char str[8192];
    sprintf(str,"### ERROR: value: %f\n", value);
    throw CoinError(str, "setInfinity", "CoinLpIO", __FILE__, __LINE__);
  }
}

/************************************************************************/
double CoinLpIO::getEpsilon() const
{
  return epsilon_;
}

/************************************************************************/
void CoinLpIO::setEpsilon(const double value) 
{
  if (value < 0.1) {
    epsilon_ = value;
  } 
  else {
    char str[8192];
    sprintf(str,"### ERROR: value: %f\n", value);
    throw CoinError(str, "setEpsilon", "CoinLpIO", __FILE__, __LINE__);
  }
}

/************************************************************************/
int CoinLpIO::getNumberAcross() const
{
  return numberAcross_;
}

/************************************************************************/
void CoinLpIO::setNumberAcross(const int value) 
{
  if (value > 0) {
    numberAcross_ = value;
  } 
  else {
    char str[8192];
    sprintf(str,"### ERROR: value: %d\n", value);
    throw CoinError(str, "setNumberAcross", "CoinLpIO", __FILE__, __LINE__);
  }
}

/************************************************************************/
int CoinLpIO::getDecimals() const
{
  return decimals_;
}

/************************************************************************/
void CoinLpIO::setDecimals(const int value) 
{
  if (value > 0) {
    decimals_ = value;
  } 
  else {
    char str[8192];
    sprintf(str,"### ERROR: value: %d\n", value);
    throw CoinError(str, "setDecimals", "CoinLpIO", __FILE__, __LINE__);
  }
}

/************************************************************************/
double CoinLpIO::objectiveOffset() const
{
  return objectiveOffset_;
}

/************************************************************************/
bool CoinLpIO::isInteger(int columnNumber) const
{
  const char * intType = integerType_;
  if (intType == NULL) return false;
  assert (columnNumber >= 0 && columnNumber < numberColumns_);
  if (intType[columnNumber] != 0) return true;
  return false;
}

/************************************************************************/
const char * CoinLpIO::integerColumns() const
{
  return integerType_;
}

/************************************************************************/
void
CoinLpIO::setLpDataWithoutRowAndColNames(
			      const CoinPackedMatrix& m,
			      const double *collb, const double *colub,
			      const double *obj_coeff,
			      const char *is_integer,
			      const double *rowlb, const double *rowub) {

  freeAll();
  problemName_ = CoinStrdup("");

  if (m.isColOrdered()) {
    matrixByRow_ = new CoinPackedMatrix();
    matrixByRow_->reverseOrderedCopyOf(m);
  } 
  else {
    matrixByRow_ = new CoinPackedMatrix(m);
  }
  numberColumns_ = matrixByRow_->getNumCols();
  numberRows_ = matrixByRow_->getNumRows();
  
  rowlower_ = reinterpret_cast<double *> (malloc (numberRows_ * sizeof(double)));
  rowupper_ = reinterpret_cast<double *> (malloc (numberRows_ * sizeof(double)));
  collower_ = reinterpret_cast<double *> (malloc (numberColumns_ * sizeof(double)));
  colupper_ = reinterpret_cast<double *> (malloc (numberColumns_ * sizeof(double)));
  objective_ = reinterpret_cast<double *> (malloc (numberColumns_ * sizeof(double)));
  std::copy(rowlb, rowlb + numberRows_, rowlower_);
  std::copy(rowub, rowub + numberRows_, rowupper_);
  std::copy(collb, collb + numberColumns_, collower_);
  std::copy(colub, colub + numberColumns_, colupper_);
  std::copy(obj_coeff, obj_coeff + numberColumns_, objective_);

  if (is_integer) {
    integerType_ = reinterpret_cast<char *> (malloc (numberColumns_ * sizeof(char)));
    std::copy(is_integer, is_integer + numberColumns_, integerType_);
  } 
  else {
    integerType_ = 0;
  }

  if((numberHash_[0] > 0) && (numberHash_[0] != numberRows_+1)) {
    stopHash(0);
  }
  if((numberHash_[1] > 0) && (numberHash_[1] != numberColumns_)) {
    stopHash(1);
  }
} /* SetLpDataWithoutRowAndColNames */

/*************************************************************************/
void CoinLpIO::setDefaultRowNames() {

  int i, nrow = getNumRows();
  char **defaultRowNames = reinterpret_cast<char **> (malloc ((nrow+1) * sizeof(char *)));
  char buff[1024];

  for(i=0; i<nrow; i++) {
    sprintf(buff, "cons%d", i);
    defaultRowNames[i] = CoinStrdup(buff);
  }
  sprintf(buff, "obj");
  defaultRowNames[nrow] = CoinStrdup(buff);

  stopHash(0);
  startHash(defaultRowNames, nrow+1, 0);
  objName_ = CoinStrdup("obj");

  for(i=0; i<nrow+1; i++) {
    free(defaultRowNames[i]);
  }
  free(defaultRowNames);

} /* setDefaultRowNames */

/*************************************************************************/
void CoinLpIO::setDefaultColNames() {

  int j, ncol = getNumCols();
  char **defaultColNames = reinterpret_cast<char **> (malloc (ncol * sizeof(char *)));
  char buff[256];

  for(j=0; j<ncol; j++) {
    sprintf(buff, "x%d", j);
    defaultColNames[j] = CoinStrdup(buff);
  }
  stopHash(1);
  startHash(defaultColNames, ncol, 1);

  for(j=0; j<ncol; j++) {
    free(defaultColNames[j]);
  }
  free(defaultColNames);

} /* setDefaultColNames */

/*************************************************************************/
void CoinLpIO::setLpDataRowAndColNames(char const * const * const rownames,
				       char const * const * const colnames) {

  int nrow = getNumRows();
  int ncol = getNumCols();

  if(rownames != NULL) {
    if(are_invalid_names(rownames, nrow+1, true)) {
      setDefaultRowNames();
      handler_->message(COIN_GENERAL_WARNING,messages_)<<
      "### CoinLpIO::setLpDataRowAndColNames(): Invalid row names\nUse getPreviousNames() to get the old row names.\nNow using default row names."
						<<CoinMessageEol;
    } 
    else {
      stopHash(0);
      startHash(rownames, nrow+1, 0);
      objName_ = CoinStrdup(rownames[nrow]);
      checkRowNames();
    }
  }
  else {
    if(objName_ == NULL) {
      objName_ = CoinStrdup("obj");      
    }
  }

  if(colnames != NULL) {
    if(are_invalid_names(colnames, ncol, false)) {
      setDefaultColNames();
      handler_->message(COIN_GENERAL_WARNING,messages_)<<
      "### CoinLpIO::setLpDataRowAndColNames(): Invalid column names\nNow using default row names."
						<<CoinMessageEol;
    } 
    else {
      stopHash(1);
      startHash(colnames, ncol, 1);
      checkColNames();
    }
  }
} /* setLpDataColAndRowNames */

/************************************************************************/
void
CoinLpIO::out_coeff(FILE *fp, const double v, const int print_1) const {

  double lp_eps = getEpsilon();

  if(!print_1) {
    if(fabs(v-1) < lp_eps) {
      return;
    }
    if(fabs(v+1) < lp_eps) {
      fprintf(fp, " -");
      return;
    }
  }

  double frac = v - floor(v);

  if(frac < lp_eps) {
    fprintf(fp, " %.0f", floor(v));
  }
  else {
    if(frac > 1 - lp_eps) {
      fprintf(fp, " %.0f", floor(v+0.5));
    }
    else {
      int decimals = getDecimals();
      char form[15];
      sprintf(form, " %%.%df", decimals);
      fprintf(fp, form, v);
    }
  }
} /* out_coeff */

/************************************************************************/
int
CoinLpIO::writeLp(const char *filename, const double epsilon,
		  const int numberAcross, const int decimals,
		  const bool useRowNames) {

  FILE *fp = NULL;
  fp = fopen(filename,"w");
  if (!fp) {
    char str[8192];
    sprintf(str,"### ERROR: unable to open file %s\n", filename);
    throw CoinError(str, "writeLP", "CoinLpIO", __FILE__, __LINE__);
  }
  int nerr = writeLp(fp, epsilon, numberAcross, decimals, useRowNames);
  fclose(fp);
  return(nerr);
}

/************************************************************************/
int
CoinLpIO::writeLp(FILE *fp, const double epsilon,
		  const int numberAcross, const int decimals,
		  const bool useRowNames) {

  setEpsilon(epsilon);
  setNumberAcross(numberAcross);
  setDecimals(decimals);
  return writeLp(fp, useRowNames);
}

/************************************************************************/
int
CoinLpIO::writeLp(const char *filename, const bool useRowNames)
{
  FILE *fp = NULL;
  fp = fopen(filename,"w");
  if (!fp) {
    char str[8192];
    sprintf(str,"### ERROR: unable to open file %s\n", filename);
    throw CoinError(str, "writeLP", "CoinLpIO", __FILE__, __LINE__);
  }
  int nerr = writeLp(fp, useRowNames);
  fclose(fp);
  return(nerr);
}

/************************************************************************/
int
CoinLpIO::writeLp(FILE *fp, const bool useRowNames)
{
   double lp_eps = getEpsilon();
   double lp_inf = getInfinity();
   int numberAcross = getNumberAcross();

   int i, j, cnt_print, loc_row_names = 0, loc_col_names = 0;
   char **prowNames = NULL, **pcolNames = NULL;

   const int *indices = matrixByRow_->getIndices();
   const double *elements  = matrixByRow_->getElements();
   int ncol = getNumCols();
   int nrow = getNumRows();
   const double *collow = getColLower();
   const double *colup = getColUpper();
   const double *rowlow = getRowLower();
   const double *rowup = getRowUpper();
   const double *obj = getObjCoefficients();
   const char *integerType = integerColumns();
   char const * const * rowNames = getRowNames();
   char const * const * colNames = getColNames();
   
   char buff[256];

   if(rowNames == NULL) {
     loc_row_names = 1;
     prowNames = reinterpret_cast<char **> (malloc ((nrow+1) * sizeof(char *)));

     for (j=0; j<nrow; j++) {
       sprintf(buff, "cons%d", j);
       prowNames[j] = CoinStrdup(buff);
     }
     prowNames[nrow] = CoinStrdup("obj");
     rowNames = prowNames;
   }

   if(colNames == NULL) {
     loc_col_names = 1;
     pcolNames = reinterpret_cast<char **> (malloc (ncol * sizeof(char *)));

     for (j=0; j<ncol; j++) {
       sprintf(buff, "x%d", j);
       pcolNames[j] = CoinStrdup(buff);
     }
     colNames = pcolNames;
   }

#ifdef LPIO_DEBUG
   printf("CoinLpIO::writeLp(): numberRows: %d numberColumns: %d\n", 
	  nrow, ncol);
#endif
 
   fprintf(fp, "\\Problem name: %s\n\n", getProblemName());
   fprintf(fp, "Minimize\n");

   if(useRowNames) {
     fprintf(fp, "%s:", objName_);
   }

   cnt_print = 0;
   for(j=0; j<ncol; j++) {
     if((cnt_print > 0) && (objective_[j] > lp_eps)) {
       fprintf(fp, " +");
     }
     if(fabs(obj[j]) > lp_eps) {
       out_coeff(fp, obj[j], 0);
       fprintf(fp, " %s", colNames[j]);
       cnt_print++;
       if(cnt_print % numberAcross == 0) {
	 fprintf(fp, "\n");
       }
     }
   }
 
   if((cnt_print > 0) && (objectiveOffset_ > lp_eps)) {
     fprintf(fp, " +");
   }
   if(fabs(objectiveOffset_) > lp_eps) {
     out_coeff(fp, objectiveOffset_, 1);
     cnt_print++;
   }

   if((cnt_print == 0) || (cnt_print % numberAcross != 0)) {
     fprintf(fp, "\n");
   }
   
   fprintf(fp, "Subject To\n");
   
   int cnt_out_rows = 0;

   for(i=0; i<nrow; i++) {
     cnt_print = 0;
     
     if(useRowNames) {
       fprintf(fp, "%s: ", rowNames[i]);
     }
     cnt_out_rows++;

     for(j=matrixByRow_->getVectorFirst(i); 
	 j<matrixByRow_->getVectorLast(i); j++) {
       if((cnt_print > 0) && (elements[j] > lp_eps)) {
	 fprintf(fp, " +");
       }
       if(fabs(elements[j]) > lp_eps) {
	 out_coeff(fp, elements[j], 0);
	 fprintf(fp, " %s", colNames[indices[j]]);
	 cnt_print++;
	 if(cnt_print % numberAcross == 0) {
	   fprintf(fp, "\n");
	 }
       }
     }

     if(rowup[i] - rowlow[i] < lp_eps) {
          fprintf(fp, " =");
	  out_coeff(fp, rowlow[i], 1);
	  fprintf(fp, "\n");
     }
     else {
       if(rowup[i] < lp_inf) {
	 fprintf(fp, " <=");
	 out_coeff(fp, rowup[i], 1);	 
	 fprintf(fp, "\n");

	 if(rowlower_[i] > -lp_inf) {

	   cnt_print = 0;

	   if(useRowNames) {
	     fprintf(fp, "%s_low:", rowNames[i]);
	   }
	   cnt_out_rows++;
	   
	   for(j=matrixByRow_->getVectorFirst(i); 
	       j<matrixByRow_->getVectorLast(i); j++) {
	     if((cnt_print>0) && (elements[j] > lp_eps)) {
	       fprintf(fp, " +");
	     }
	     if(fabs(elements[j]) > lp_eps) {
	       out_coeff(fp, elements[j], 0);
	       fprintf(fp, " %s", colNames[indices[j]]);
	       cnt_print++;
	       if(cnt_print % numberAcross == 0) {
		 fprintf(fp, "\n");
	       }
	     }
	   }
	   fprintf(fp, " >=");
	   out_coeff(fp, rowlow[i], 1);
	   fprintf(fp, "\n");
	 }

       }
       else {
	 fprintf(fp, " >=");
	 out_coeff(fp, rowlow[i], 1);	 
	 fprintf(fp, "\n");
       }
     }
   }

#ifdef LPIO_DEBUG
   printf("CoinLpIO::writeLp(): Done with constraints\n");
#endif

   fprintf(fp, "Bounds\n");
   
   for(j=0; j<ncol; j++) {
     if((collow[j] > -lp_inf) && (colup[j] < lp_inf)) {
       out_coeff(fp, collow[j], 1);
       fprintf(fp, " <= %s <=", colNames[j]); 
       out_coeff(fp, colup[j], 1);
       fprintf(fp, "\n");
     }
     if((collow[j] == -lp_inf) && (colup[j] < lp_inf)) {
       fprintf(fp, "%s <=", colNames[j]);
       out_coeff(fp, colup[j], 1);
       fprintf(fp, "\n");
     }
     if((collow[j] > -lp_inf) && (colup[j] == lp_inf)) {
       if(fabs(collow[j]) > lp_eps) { 
	 out_coeff(fp, collow[j], 1);
	 fprintf(fp, " <= %s\n", colNames[j]);
       }
     }
     if(collow[j] == -lp_inf) {
       fprintf(fp, " %s Free\n", colNames[j]); 
     }
   }

#ifdef LPIO_DEBUG
   printf("CoinLpIO::writeLp(): Done with bounds\n");
#endif

   if(integerType != NULL) {
     int first_int = 1;
     cnt_print = 0;
     for(j=0; j<ncol; j++) {
       if(integerType[j] == 1) {

	 if(first_int) {
	   fprintf(fp, "Integers\n");
	   first_int = 0;
	 }

	 fprintf(fp, "%s ", colNames[j]);
	 cnt_print++;
	 if(cnt_print % numberAcross == 0) {
	   fprintf(fp, "\n");
	 }
       }
     }

     if(cnt_print % numberAcross != 0) {
       fprintf(fp, "\n");
     }
   }

#ifdef LPIO_DEBUG
   printf("CoinLpIO::writeLp(): Done with integers\n");
#endif

   fprintf(fp, "End\n");

   if(loc_row_names) {
     for(j=0; j<nrow+1; j++) {
       free(prowNames[j]);
     }
     free(prowNames);
   }

   if(loc_col_names) {
     for(j=0; j<ncol; j++) {
       free(pcolNames[j]);
     }
     free(pcolNames);
   }
   return 0;
} /* writeLp */

/*************************************************************************/
int 
CoinLpIO::find_obj(FILE *fp) const {

  char buff[1024];

  sprintf(buff, "aa");
  size_t lbuff = strlen(buff);

  while(((lbuff != 8) || (CoinStrNCaseCmp(buff, "minimize", 8) != 0)) &&
	((lbuff != 3) || (CoinStrNCaseCmp(buff, "min", 3) != 0)) &&
	((lbuff != 8) || (CoinStrNCaseCmp(buff, "maximize", 8) != 0)) &&
	((lbuff != 3) || (CoinStrNCaseCmp(buff, "max", 3) != 0))) {

    scan_next(buff, fp);
    lbuff = strlen(buff);
    
    if(feof(fp)) {
      char str[8192];
      sprintf(str,"### ERROR: Unable to locate objective function\n");
      throw CoinError(str, "find_obj", "CoinLpIO", __FILE__, __LINE__);
    }
  }

  if(((lbuff == 8) && (CoinStrNCaseCmp(buff, "minimize", 8) == 0)) ||
     ((lbuff == 3) && (CoinStrNCaseCmp(buff, "min", 3) == 0))) {
    return(1);
  }
  return(-1);
} /* find_obj */

/*************************************************************************/
int
CoinLpIO::is_subject_to(const char *buff) const {

  size_t lbuff = strlen(buff);

  if(((lbuff == 4) && (CoinStrNCaseCmp(buff, "s.t.", 4) == 0)) ||
     ((lbuff == 3) && (CoinStrNCaseCmp(buff, "st.", 3) == 0)) ||
     ((lbuff == 2) && (CoinStrNCaseCmp(buff, "st", 2) == 0))) {
    return(1);
  }
  if((lbuff == 7) && (CoinStrNCaseCmp(buff, "subject", 7) == 0)) {
    return(2);
  }
  return(0);
} /* is_subject_to */

/*************************************************************************/
int 
CoinLpIO::first_is_number(const char *buff) const {

  size_t pos;
  char str_num[] = "1234567890";

  pos = strcspn (buff, str_num);
  if (pos == 0) {
    return(1);
  }
  return(0);
} /* first_is_number */

/*************************************************************************/
int 
CoinLpIO::is_sense(const char *buff) const {

  size_t pos;
  char str_sense[] = "<>=";

  pos = strcspn(buff, str_sense);
  if(pos == 0) {
    if(strcmp(buff, "<=") == 0) {
      return(0);
    }
    if(strcmp(buff, "=") == 0) {
      return(1);
    }
    if(strcmp(buff, ">=") == 0) {
      return(2);
    }
    
    printf("### ERROR: CoinLpIO: is_sense(): string: %s \n", buff);
  }
  return(-1);
} /* is_sense */

/*************************************************************************/
int 
CoinLpIO::is_free(const char *buff) const {

  size_t lbuff = strlen(buff);

  if((lbuff == 4) && (CoinStrNCaseCmp(buff, "free", 4) == 0)) {
    return(1);
  }
  return(0);
} /* is_free */

/*************************************************************************/
int 
CoinLpIO::is_inf(const char *buff) const {

  size_t lbuff = strlen(buff);

  if((lbuff == 3) && (CoinStrNCaseCmp(buff, "inf", 3) == 0)) {
    return(1);
  }
  return(0);
} /* is_inf */

/*************************************************************************/
int 
CoinLpIO::is_comment(const char *buff) const {

  if((buff[0] == '/') || (buff[0] == '\\')) {
    return(1);
  }
  return(0);
} /* is_comment */

/*************************************************************************/
void
CoinLpIO::skip_comment(char *buff, FILE *fp) const {

  while(strcspn(buff, "\n") == strlen(buff)) { // end of line not read yet
    if(feof(fp)) {
      char str[8192];
      sprintf(str,"### ERROR: end of file reached while skipping comment\n");
      throw CoinError(str, "skip_comment", "CoinLpIO", __FILE__, __LINE__);
    }
    if(ferror(fp)) {
      char str[8192];
      sprintf(str,"### ERROR: error while skipping comment\n");
      throw CoinError(str, "skip_comment", "CoinLpIO", __FILE__, __LINE__);
    }
    char * x=fgets(buff, sizeof(buff), fp);    
    if (!x)
      throw("bad fgets");
  } 
} /* skip_comment */

/*************************************************************************/
void
CoinLpIO::scan_next(char *buff, FILE *fp) const {

  int x=fscanf(fp, "%s", buff);
  if (x<=0)
    throw("bad fscanf");
  while(is_comment(buff)) {
    skip_comment(buff, fp);
    x=fscanf(fp, "%s", buff);
    if (x<=0)
      throw("bad fscanf");
  }

#ifdef LPIO_DEBUG
  printf("CoinLpIO::scan_next: (%s)\n", buff);
#endif

} /* scan_next */

/*************************************************************************/
int 
CoinLpIO::is_invalid_name(const char *name, 
			  const bool ranged) const {

  size_t pos, lname, valid_lname = 100;
  char str_valid[] = "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\"!#$%&(),.;?@_'`{}~";

  if(ranged) {
    valid_lname -= 4; // will add "_low" when writing the Lp file
  }

  if(name == NULL) {
    lname = 0;
  }
  else {
    lname = strlen(name);
  }
  if(lname < 1) {
    handler_->message(COIN_GENERAL_WARNING,messages_)<<
    "### CoinLpIO::is_invalid_name(): Name is empty"
					      <<CoinMessageEol;
    return(5);
  }
  if(lname > valid_lname) {
	char printBuffer[512];
	sprintf(printBuffer,"### CoinLpIO::is_invalid_name(): Name %s is too long", 
	   name);
	handler_->message(COIN_GENERAL_WARNING,messages_)<<printBuffer
							 <<CoinMessageEol;
    return(1);
  }
  if(first_is_number(name)) {
    char printBuffer[512];
    sprintf(printBuffer,"### CoinLpIO::is_invalid_name(): Name %s should not start with a number", name);
    handler_->message(COIN_GENERAL_WARNING,messages_)<<printBuffer
						     <<CoinMessageEol;
    return(2);
  }
  pos = strspn(name, str_valid);
  if(pos != lname) {
    char printBuffer[512];
    sprintf(printBuffer,"### CoinLpIO::is_invalid_name(): Name %s contains illegal character '%c'", name, name[pos]);
    handler_->message(COIN_GENERAL_WARNING,messages_)<<printBuffer
						     <<CoinMessageEol;
    return(3);
  }

  if((is_keyword(name)) || (is_free(name) || (is_inf(name)))) {
    return(4);
  }

  return(0);
} /* is_invalid_name */

/*************************************************************************/
int 
CoinLpIO::are_invalid_names(char const * const * const vnames, 
			    const int card_vnames, 
			    const bool check_ranged) const {

  int i, invalid = 0, flag, nrows = getNumRows();
  bool is_ranged = 0;
  const char * rSense = getRowSense();

  if((check_ranged) && (card_vnames != nrows+1)) {
    char str[8192];
    sprintf(str,"### ERROR: card_vnames: %d   number of rows: %d\n",
	    card_vnames, getNumRows());
    throw CoinError(str, "are_invalid_names", "CoinLpIO", __FILE__, __LINE__);
  }

  for(i=0; i<card_vnames; i++) {

    if((check_ranged) && (i < nrows) && (rSense[i] == 'R')) {
      is_ranged = true;
    }
    else {
      is_ranged = false;
    }
    flag = is_invalid_name(vnames[i], is_ranged);
    if(flag) {
      char printBuffer[512];
      sprintf(printBuffer,"### CoinLpIO::are_invalid_names(): Invalid name: vnames[%d]: %s",
	      i, vnames[i]);
      handler_->message(COIN_GENERAL_WARNING,messages_)<<printBuffer
						       <<CoinMessageEol;
      invalid = flag;
    }
  }
  return(invalid);
} /* are_invalid_names */

/*************************************************************************/
int 
CoinLpIO::read_monom_obj(FILE *fp, double *coeff, char **name, int *cnt, 
						 char **obj_name) {

  double mult;
  char buff[1024] = "aa", loc_name[1024], *start;
  int read_st = 0;

  scan_next(buff, fp);

  if(feof(fp)) {
    char str[8192];
    sprintf(str,"### ERROR: Unable to read objective function\n");
    throw CoinError(str, "read_monom_obj", "CoinLpIO", __FILE__, __LINE__);
  }

  if(buff[strlen(buff)-1] == ':') {
    buff[strlen(buff)-1] = '\0';

#ifdef LPIO_DEBUG
    printf("CoinLpIO: read_monom_obj(): obj_name: %s\n", buff);
#endif

    *obj_name = CoinStrdup(buff);
    return(0);
  }


  read_st = is_subject_to(buff);

#ifdef LPIO_DEBUG
  printf("read_monom_obj: first buff: (%s)\n", buff);
#endif

  if(read_st > 0) {
    return(read_st);
  }

  start = buff;
  mult = 1;
  if(buff[0] == '+') {
    mult = 1;
    if(strlen(buff) == 1) {
      scan_next(buff, fp);
      start = buff;
    }
    else {
      start = &(buff[1]);
    }
  }
  
  if(buff[0] == '-') {
    mult = -1;
    if(strlen(buff) == 1) {
      scan_next(buff, fp);
      start = buff;
    }
    else {
      start = &(buff[1]);
    }
  }
  
  if(first_is_number(start)) {
    coeff[*cnt] = atof(start);       
    sprintf(loc_name, "aa");
    scan_next(loc_name, fp);
  }
  else {
    coeff[*cnt] = 1;
    strcpy(loc_name, start);
  }

  read_st = is_subject_to(loc_name);

#ifdef LPIO_DEBUG
  printf("read_monom_obj: second buff: (%s)\n", buff);
#endif

  if(read_st > 0) {
    setObjectiveOffset(mult * coeff[*cnt]);

#ifdef LPIO_DEBUG
  printf("read_monom_obj: objectiveOffset: %f\n", objectiveOffset_);
#endif

    return(read_st);
  }

  coeff[*cnt] *= mult;
  name[*cnt] = CoinStrdup(loc_name);

#ifdef LPIO_DEBUG
  printf("read_monom_obj: (%f)  (%s)\n", coeff[*cnt], name[*cnt]);
#endif

  (*cnt)++;

  return(read_st);
} /* read_monom_obj */

/*************************************************************************/
int 
CoinLpIO::read_monom_row(FILE *fp, char *start_str, 
			 double *coeff, char **name, 
			 int cnt_coeff) const {

  double mult;
  char buff[1024], loc_name[1024], *start;
  int read_sense = -1;

  sprintf(buff, "%s", start_str);
  read_sense = is_sense(buff);
  if(read_sense > -1) {
    return(read_sense);
  }

  start = buff;
  mult = 1;
  if(buff[0] == '+') {
    mult = 1;
    if(strlen(buff) == 1) {
      scan_next(buff, fp);
      start = buff;
    }
    else {
      start = &(buff[1]);
    }
  }
  
  if(buff[0] == '-') {
    mult = -1;
    if(strlen(buff) == 1) {
      scan_next(buff, fp);
      start = buff;
    }
    else {
      start = &(buff[1]);
    }
  }
  
  if(first_is_number(start)) {
    coeff[cnt_coeff] = atof(start);       
    scan_next(loc_name, fp);
  }
  else {
    coeff[cnt_coeff] = 1;
    strcpy(loc_name, start);
  }

  coeff[cnt_coeff] *= mult;
#ifdef KILL_ZERO_READLP
  if (fabs(coeff[cnt_coeff])>epsilon_)
    name[cnt_coeff] = CoinStrdup(loc_name);
  else
    read_sense=-2; // effectively zero
#else
  name[cnt_coeff] = CoinStrdup(loc_name);
#endif

#ifdef LPIO_DEBUG
  printf("CoinLpIO: read_monom_row: (%f)  (%s)\n", 
	 coeff[cnt_coeff], name[cnt_coeff]);
#endif  
  return(read_sense);
} /* read_monom_row */

/*************************************************************************/
void
CoinLpIO::realloc_coeff(double **coeff, char ***colNames, 
			int *maxcoeff) const {
  
  *maxcoeff *= 5;

  *colNames = reinterpret_cast<char **> (realloc ((*colNames), (*maxcoeff+1) * sizeof(char *)));
  *coeff = reinterpret_cast<double *> (realloc ((*coeff), (*maxcoeff+1) * sizeof(double)));

} /* realloc_coeff */

/*************************************************************************/
void
CoinLpIO::realloc_row(char ***rowNames, int **start, double **rhs, 
		      double **rowlow, double **rowup, int *maxrow) const {

  *maxrow *= 5;
  *rowNames = reinterpret_cast<char **> (realloc ((*rowNames), (*maxrow+1) * sizeof(char *)));
  *start = reinterpret_cast<int *> (realloc ((*start), (*maxrow+1) * sizeof(int)));
  *rhs = reinterpret_cast<double *> (realloc ((*rhs), (*maxrow+1) * sizeof(double)));
  *rowlow = reinterpret_cast<double *> (realloc ((*rowlow), (*maxrow+1) * sizeof(double)));
  *rowup = reinterpret_cast<double *> (realloc ((*rowup), (*maxrow+1) * sizeof(double)));

} /* realloc_row */

/*************************************************************************/
void
CoinLpIO::realloc_col(double **collow, double **colup, char **is_int,
		      int *maxcol) const {
  
  *maxcol += 100;
  *collow = reinterpret_cast<double *> (realloc ((*collow), (*maxcol+1) * sizeof(double)));
  *colup = reinterpret_cast<double *> (realloc ((*colup), (*maxcol+1) * sizeof(double)));
  *is_int = reinterpret_cast<char *> (realloc ((*is_int), (*maxcol+1) * sizeof(char)));

} /* realloc_col */

/*************************************************************************/
void 
CoinLpIO::read_row(FILE *fp, char *buff,
		   double **pcoeff, char ***pcolNames, 
		   int *cnt_coeff,
		   int *maxcoeff,
		   double *rhs, double *rowlow, double *rowup, 
		   int *cnt_row, double inf) const {

  int read_sense = -1;
  char start_str[1024];
  
  sprintf(start_str, "%s", buff);

  while(read_sense < 0) {

    if((*cnt_coeff) == (*maxcoeff)) {
      realloc_coeff(pcoeff, pcolNames, maxcoeff);
    }
    read_sense = read_monom_row(fp, start_str, 
				*pcoeff, *pcolNames, *cnt_coeff);
#ifdef KILL_ZERO_READLP
    if (read_sense!=-2) // see if zero
#endif
      (*cnt_coeff)++;

    scan_next(start_str, fp);

    if(feof(fp)) {
      char str[8192];
      sprintf(str,"### ERROR: Unable to read row monomial\n");
      throw CoinError(str, "read_monom_row", "CoinLpIO", __FILE__, __LINE__);
    }
  }
  (*cnt_coeff)--;

  rhs[*cnt_row] = atof(start_str);

  switch(read_sense) {
  case 0: rowlow[*cnt_row] = -inf; rowup[*cnt_row] = rhs[*cnt_row];
    break;
  case 1: rowlow[*cnt_row] = rhs[*cnt_row]; rowup[*cnt_row] = rhs[*cnt_row];
    break;
  case 2: rowlow[*cnt_row] = rhs[*cnt_row]; rowup[*cnt_row] = inf; 
    break;
  default: break;
  }
  (*cnt_row)++;

} /* read_row */

/*************************************************************************/
int 
CoinLpIO::is_keyword(const char *buff) const {

  size_t lbuff = strlen(buff);

  if(((lbuff == 5) && (CoinStrNCaseCmp(buff, "bound", 5) == 0)) ||
     ((lbuff == 6) && (CoinStrNCaseCmp(buff, "bounds", 6) == 0))) {
    return(1);
  }

  if(((lbuff == 7) && (CoinStrNCaseCmp(buff, "integer", 7) == 0)) ||
     ((lbuff == 8) && (CoinStrNCaseCmp(buff, "integers", 8) == 0))) {
    return(2);
  }
  
  if(((lbuff == 7) && (CoinStrNCaseCmp(buff, "general", 7) == 0)) ||
     ((lbuff == 8) && (CoinStrNCaseCmp(buff, "generals", 8) == 0))) {
    return(2);
  }

  if(((lbuff == 6) && (CoinStrNCaseCmp(buff, "binary", 6) == 0)) ||
     ((lbuff == 8) && (CoinStrNCaseCmp(buff, "binaries", 8) == 0))) {
    return(3);
  }
  
  if((lbuff == 3) && (CoinStrNCaseCmp(buff, "end", 3) == 0)) {
    return(4);
  }

  return(0);

} /* is_keyword */

/*************************************************************************/
void
CoinLpIO::readLp(const char *filename, const double epsilon)
{
  setEpsilon(epsilon);
  readLp(filename);
}

/*************************************************************************/
void
CoinLpIO::readLp(const char *filename)
{
  FILE *fp = fopen(filename, "r");
  if(!fp) {
    char str[8192];
    sprintf(str,"### ERROR: Unable to open file %s for reading\n", filename);
    throw CoinError(str, "readLp", "CoinLpIO", __FILE__, __LINE__);
  }
  readLp(fp);
  fclose(fp);
}

/*************************************************************************/
void
CoinLpIO::readLp(FILE* fp, const double epsilon)
{
  setEpsilon(epsilon);
  readLp(fp);
}

/*************************************************************************/
void
CoinLpIO::readLp(FILE* fp)
{

  int maxrow = 1000;
  int maxcoeff = 40000;
  double lp_eps = getEpsilon();
  double lp_inf = getInfinity();

  char buff[1024];

  int objsense, cnt_coeff = 0, cnt_row = 0, cnt_obj = 0;
  char *objName = NULL;

  char **colNames = reinterpret_cast<char **> (malloc ((maxcoeff+1) * sizeof(char *)));
  double *coeff = reinterpret_cast<double *> (malloc ((maxcoeff+1) * sizeof(double)));

  char **rowNames = reinterpret_cast<char **> (malloc ((maxrow+1) * sizeof(char *)));
  int *start = reinterpret_cast<int *> (malloc ((maxrow+1) * sizeof(int)));
  double *rhs = reinterpret_cast<double *> (malloc ((maxrow+1) * sizeof(double)));
  double *rowlow = reinterpret_cast<double *> (malloc ((maxrow+1) * sizeof(double)));
  double *rowup = reinterpret_cast<double *> (malloc ((maxrow+1) * sizeof(double)));

  int i;

  objsense = find_obj(fp);

  int read_st = 0;
  while(!read_st) {
    read_st = read_monom_obj(fp, coeff, colNames, &cnt_obj, &objName);

    if(cnt_obj == maxcoeff) {
      realloc_coeff(&coeff, &colNames, &maxcoeff);
    }
  }
  
  start[0] = cnt_obj;
  cnt_coeff = cnt_obj;

  if(read_st == 2) {
    int x=fscanf(fp, "%s", buff);
    if (x<=0)
      throw("bad fscanf");
    size_t lbuff = strlen(buff);

    if((lbuff != 2) || (CoinStrNCaseCmp(buff, "to", 2) != 0)) {
      char str[8192];
      sprintf(str,"### ERROR: Can not locate keyword 'Subject To'\n");
      throw CoinError(str, "readLp", "CoinLpIO", __FILE__, __LINE__);
    }
  }
  
  scan_next(buff, fp);

  while(!is_keyword(buff)) {
    if(buff[strlen(buff)-1] == ':') {
      buff[strlen(buff)-1] = '\0';

#ifdef LPIO_DEBUG
      printf("CoinLpIO::readLp(): rowName[%d]: %s\n", cnt_row, buff);
#endif

      rowNames[cnt_row] = CoinStrdup(buff);
      scan_next(buff, fp);
    }
    else {
      char rname[15];
      sprintf(rname, "cons%d", cnt_row); 
      rowNames[cnt_row] = CoinStrdup(rname);
    }
    read_row(fp, buff, 
	     &coeff, &colNames, &cnt_coeff, &maxcoeff, rhs, rowlow, rowup, 
	     &cnt_row, lp_inf);
    scan_next(buff, fp);
    start[cnt_row] = cnt_coeff;

    if(cnt_row == maxrow) {
      realloc_row(&rowNames, &start, &rhs, &rowlow, &rowup, &maxrow);
    }

  }

  numberRows_ = cnt_row;

  stopHash(1);
  startHash(colNames, cnt_coeff, 1);
  
  COINColumnIndex icol;
  int read_sense1,  read_sense2;
  double bnd1 = 0, bnd2 = 0;

  int maxcol = numberHash_[1] + 100;

  double *collow = reinterpret_cast<double *> (malloc ((maxcol+1) * sizeof(double)));
  double *colup = reinterpret_cast<double *> (malloc ((maxcol+1) * sizeof(double)));
  char *is_int = reinterpret_cast<char *> (malloc ((maxcol+1) * sizeof(char)));
  int has_int = 0;

  for (i=0; i<maxcol; i++) {
    collow[i] = 0;
    colup[i] = lp_inf;
    is_int[i] = 0;
  }

  int done = 0;

  while(!done) {
    switch(is_keyword(buff)) {

    case 1: /* Bounds section */ 
      scan_next(buff, fp);

      while(is_keyword(buff) == 0) {

	read_sense1 = -1;
	read_sense2 = -1;
	int mult = 1;
	char *start_str = buff;

	if(buff[0] == '-' || buff[0] == '+') {
	  mult = (buff[0] == '-') ? -1 : +1;
	  if(strlen(buff) == 1) {
	    scan_next(buff, fp);
	    start_str = buff;
	  }
	  else {
	    start_str = &(buff[1]);
	  }
	}

	int scan_sense = 0;
	if(first_is_number(start_str)) {
	  bnd1 = mult * atof(start_str);
	  scan_sense = 1;
	}
	else {
	  if(is_inf(start_str)) {
	    bnd1 = mult * lp_inf;
	    scan_sense = 1;
	  }
	}
	if(scan_sense) {
	  scan_next(buff, fp);
	  read_sense1 = is_sense(buff);
	  if(read_sense1 < 0) {
	    char str[8192];
	    sprintf(str,"### ERROR: Bounds; expect a sense, get: %s\n", buff);
	    throw CoinError(str, "readLp", "CoinLpIO", __FILE__, __LINE__);
	  }
	  scan_next(buff, fp);
	}

	icol = findHash(buff, 1);
	if(icol < 0) {
	  char printBuffer[512];
	  sprintf(printBuffer,"### CoinLpIO::readLp(): Variable %s does not appear in objective function or constraints", buff);
	  handler_->message(COIN_GENERAL_WARNING,messages_)<<printBuffer
							   <<CoinMessageEol;
	  insertHash(buff, 1);
	  icol = findHash(buff, 1);
	  if(icol == maxcol) {
	    realloc_col(&collow, &colup, &is_int, &maxcol);
	  }
	}

	scan_next(buff, fp);
	if(is_free(buff)) {
	  collow[icol] = -lp_inf;
	  scan_next(buff, fp);
	}
       	else {
	  read_sense2 = is_sense(buff);
	  if(read_sense2 > -1) {
	    scan_next(buff, fp);
	    mult = 1;
	    start_str = buff;

	    if(buff[0] == '-'||buff[0] == '+') {
	      mult = (buff[0] == '-') ? -1 : +1;
	      if(strlen(buff) == 1) {
		scan_next(buff, fp);
		start_str = buff;
	      }
	      else {
		start_str = &(buff[1]);
	      }
	    }
	    if(first_is_number(start_str)) {
	      bnd2 = mult * atof(start_str);
	      scan_next(buff, fp);
	    }
	    else {
	      if(is_inf(start_str)) {
		bnd2 = mult * lp_inf;
		scan_next(buff, fp);
	      }
	      else {
		char str[8192];
		sprintf(str,"### ERROR: Bounds; expect a number, get: %s\n",
			buff);
		throw CoinError(str, "readLp", "CoinLpIO", __FILE__, __LINE__);
	      }
	    }
	  }

	  if((read_sense1 > -1) && (read_sense2 > -1)) {
	    if(read_sense1 != read_sense2) {
	      char str[8192];
	      sprintf(str,"### ERROR: Bounds; variable: %s read_sense1: %d  read_sense2: %d\n",
		      buff, read_sense1, read_sense2);
	      throw CoinError(str, "readLp", "CoinLpIO", __FILE__, __LINE__);
	    }
	    else {
	      if(read_sense1 == 1) {
		if(fabs(bnd1 - bnd2) > lp_eps) {
		  char str[8192];
		  sprintf(str,"### ERROR: Bounds; variable: %s read_sense1: %d  read_sense2: %d  bnd1: %f  bnd2: %f\n", 
			  buff, read_sense1, read_sense2, bnd1, bnd2);
		  throw CoinError(str, "readLp", "CoinLpIO", __FILE__, __LINE__);
		}
		collow[icol] = bnd1;
		colup[icol] = bnd1;
	      }
	      if(read_sense1 == 0) {
		collow[icol] = bnd1;
		colup[icol] = bnd2;	    
	      }
	      if(read_sense1 == 2) {
		colup[icol] = bnd1;
		collow[icol] = bnd2;	    
	      }
	    }
	  }
	  else {
	    if(read_sense1 > -1) {
	      switch(read_sense1) {
	      case 0: collow[icol] = bnd1; break;
	      case 1: collow[icol] = bnd1; colup[icol] = bnd1; break;
	      case 2: colup[icol] = bnd1; break;
	      }
	    }
	    if(read_sense2 > -1) {
	      switch(read_sense2) {
	      case 0: colup[icol] = bnd2; break;
	      case 1: collow[icol] = bnd2; colup[icol] = bnd2; break;
	      case 2: collow[icol] = bnd2; break;
	      }
	    }
	  }
	}
      }
      break;

    case 2: /* Integers/Generals section */

      scan_next(buff, fp);
    
      while(is_keyword(buff) == 0) {
      
	icol = findHash(buff, 1);

#ifdef LPIO_DEBUG
	printf("CoinLpIO::readLp(): Integer: colname: (%s)  icol: %d\n", 
	       buff, icol);
#endif

	if(icol < 0) {
	  char printBuffer[512];
	  sprintf(printBuffer,"### CoinLpIO::readLp(): Integer variable %s does not appear in objective function or constraints", buff);
	  handler_->message(COIN_GENERAL_WARNING,messages_)<<printBuffer
							   <<CoinMessageEol;
	  insertHash(buff, 1);
	  icol = findHash(buff, 1);
	  if(icol == maxcol) {
	    realloc_col(&collow, &colup, &is_int, &maxcol);
	  }
	  
#ifdef LPIO_DEBUG
	  printf("CoinLpIO::readLp(): Integer: colname: (%s)  icol: %d\n", 
		 buff, icol);
#endif
	  
	}
	is_int[icol] = 1;
	has_int = 1;
	scan_next(buff, fp);
      };
      break;

    case 3: /* Binaries section */
  
      scan_next(buff, fp);
      
      while(is_keyword(buff) == 0) {

	icol = findHash(buff, 1);

#ifdef LPIO_DEBUG
	printf("CoinLpIO::readLp(): binary: colname: (%s)  icol: %d\n", 
	       buff, icol);
#endif

	if(icol < 0) {
	  char printBuffer[512];
	  sprintf(printBuffer,"### CoinLpIO::readLp(): Binary variable %s does not appear in objective function or constraints", buff);
	  handler_->message(COIN_GENERAL_WARNING,messages_)<<printBuffer
							   <<CoinMessageEol;
	  insertHash(buff, 1);
	  icol = findHash(buff, 1);
	  if(icol == maxcol) {
	    realloc_col(&collow, &colup, &is_int, &maxcol);
	  }
#ifdef LPIO_DEBUG
	  printf("CoinLpIO::readLp(): binary: colname: (%s)  icol: %d\n", 
		 buff, icol);
#endif
	  
	}

	is_int[icol] = 1;
	has_int = 1;
	if(collow[icol] < 0) {
	  collow[icol] = 0;
	}
	if(colup[icol] > 1) {
	  colup[icol] = 1;
	}
	scan_next(buff, fp);
      }
      break;
      
    case 4: done = 1; break;
      
    default: 
      char str[8192];
      sprintf(str,"### ERROR: Lost while reading: (%s)\n", buff);
      throw CoinError(str, "readLp", "CoinLpIO", __FILE__, __LINE__);
      break;
    }
  }

#ifdef LPIO_DEBUG
  printf("CoinLpIO::readLp(): Done with reading the Lp file\n");
#endif

  int *ind = reinterpret_cast<int *> (malloc ((maxcoeff+1) * sizeof(int)));

  for(i=0; i<cnt_coeff; i++) {
    ind[i] = findHash(colNames[i], 1);

#ifdef LPIO_DEBUG
    printf("CoinLpIO::readLp(): name[%d]: (%s)   ind: %d\n", 
	   i, colNames[i], ind[i]);
#endif

    if(ind[i] < 0) {
      char str[8192];
      sprintf(str,"### ERROR: Hash table: %s not found\n", colNames[i]);
      throw CoinError(str, "readLp", "CoinLpIO", __FILE__, __LINE__);
    }
  }
  
  numberColumns_ = numberHash_[1];
  numberElements_ = cnt_coeff - start[0];

  double *obj = reinterpret_cast<double *> (malloc (numberColumns_ * sizeof(double)));
  memset(obj, 0, numberColumns_ * sizeof(double));

  for(i=0; i<cnt_obj; i++) {
    icol = findHash(colNames[i], 1);
    if(icol < 0) {
      char str[8192];
      sprintf(str,"### ERROR: Hash table: %s (obj) not found\n", colNames[i]);
      throw CoinError(str, "readLp", "CoinLpIO", __FILE__, __LINE__);
    }
    obj[icol] = objsense * coeff[i];
  }

  if (objsense == -1) {
    handler_->message(COIN_GENERAL_INFO,messages_)<<
      " CoinLpIO::readLp(): Maximization problem reformulated as minimization"
						  <<CoinMessageEol;
    objectiveOffset_ = -objectiveOffset_;
  }


  for(i=0; i<cnt_row+1; i++) {
    start[i] -= cnt_obj;
  }

  CoinPackedMatrix *matrix = 
    new CoinPackedMatrix(false,
			 numberColumns_, numberRows_, numberElements_,
			 &(coeff[cnt_obj]), &(ind[cnt_obj]), start, NULL);

#ifdef LPIO_DEBUG
  matrix->dumpMatrix();  
#endif

  setLpDataWithoutRowAndColNames(*matrix, collow, colup,
				 obj, has_int ? is_int : 0, rowlow, rowup);


  if(objName == NULL) {
    rowNames[cnt_row] = CoinStrdup("obj");
  }
  else {
    rowNames[cnt_row] = CoinStrdup(objName);
  }

  // Hash tables for column names are already set up
  setLpDataRowAndColNames(rowNames, NULL);

  if(are_invalid_names(names_[1], numberHash_[1], false)) {
    setDefaultColNames();
    handler_->message(COIN_GENERAL_WARNING,messages_)<<
      "### CoinLpIO::readLp(): Invalid column names\nNow using default column names."
						     <<CoinMessageEol;
  } 
  
  for(i=0; i<cnt_coeff; i++) {
    free(colNames[i]);
  }
  free(colNames);

  for(i=0; i<cnt_row+1; i++) {
    free(rowNames[i]);
  }
  free(rowNames);

  free(objName);

#ifdef LPIO_DEBUG
  writeLp("readlp.xxx");
  printf("CoinLpIO::readLp(): read Lp file written in file readlp.xxx\n");
#endif

  free(coeff);
  free(start);
  free(ind);
  free(colup);
  free(collow);
  free(rhs);
  free(rowlow);
  free(rowup);
  free(is_int);
  free(obj);
  delete matrix;

} /* readLp */

/*************************************************************************/
void
CoinLpIO::print() const {

  printf("problemName_: %s\n", problemName_);
  printf("numberRows_: %d\n", numberRows_);
  printf("numberColumns_: %d\n", numberColumns_);

  printf("matrixByRows_:\n");
  matrixByRow_->dumpMatrix();  

  int i;
  printf("rowlower_:\n");
  for(i=0; i<numberRows_; i++) {
    printf("%.5f ", rowlower_[i]);
  }
  printf("\n");

  printf("rowupper_:\n");
  for(i=0; i<numberRows_; i++) {
    printf("%.5f ", rowupper_[i]);
  }
  printf("\n");
  
  printf("collower_:\n");
  for(i=0; i<numberColumns_; i++) {
    printf("%.5f ", collower_[i]);
  }
  printf("\n");

  printf("colupper_:\n");
  for(i=0; i<numberColumns_; i++) {
    printf("%.5f ", colupper_[i]);
  }
  printf("\n");
  
  printf("objective_:\n");
  for(i=0; i<numberColumns_; i++) {
    printf("%.5f ", objective_[i]);
  }
  printf("\n");
  
  if(integerType_ != NULL) {
    printf("integerType_:\n");
    for(i=0; i<numberColumns_; i++) {
      printf("%c ", integerType_[i]);
    }
  }
  else {
    printf("integerType_: NULL\n");
  }

  printf("\n");
  if(fileName_ != NULL) {
    printf("fileName_: %s\n", fileName_);
  }
  printf("infinity_: %.5f\n", infinity_);
} /* print */


/*************************************************************************/
// Hash functions slightly modified from CoinMpsIO.cpp

namespace {
  const int mmult[] = {
    262139, 259459, 256889, 254291, 251701, 249133, 246709, 244247,
    241667, 239179, 236609, 233983, 231289, 228859, 226357, 223829,
    221281, 218849, 216319, 213721, 211093, 208673, 206263, 203773,
    201233, 198637, 196159, 193603, 191161, 188701, 186149, 183761,
    181303, 178873, 176389, 173897, 171469, 169049, 166471, 163871,
    161387, 158941, 156437, 153949, 151531, 149159, 146749, 144299,
    141709, 139369, 136889, 134591, 132169, 129641, 127343, 124853,
    122477, 120163, 117757, 115361, 112979, 110567, 108179, 105727,
    103387, 101021, 98639, 96179, 93911, 91583, 89317, 86939, 84521,
    82183, 79939, 77587, 75307, 72959, 70793, 68447, 66103
  };
 int compute_hash(const char *name, int maxsiz, int length)
{
  
  int n = 0;
  int j;

  for ( j = 0; j < length; ++j ) {
    int iname = name[j];

    n += mmult[j] * iname;
  }
  return ( abs ( n ) % maxsiz );	/* integer abs */
}
} // end file-local namespace

/************************************************************************/
//  startHash.  Creates hash list for names
//  setup names_[section] with names in the same order as in the parameter, 
//  but removes duplicates

void
CoinLpIO::startHash(char const * const * const names, 
		    const COINColumnIndex number, int section)
{
  maxHash_[section] = 4 * number;
  int maxhash = maxHash_[section];
  COINColumnIndex i, ipos, iput;

  names_[section] = reinterpret_cast<char **> (malloc(maxhash * sizeof(char *)));
  hash_[section] = new CoinHashLink[maxhash];
  
  CoinHashLink * hashThis = hash_[section];
  char **hashNames = names_[section];
  
  for ( i = 0; i < maxhash; i++ ) {
    hashThis[i].index = -1;
    hashThis[i].next = -1;
  }
  
  /*
   * Initialize the hash table.  Only the index of the first name that
   * hashes to a value is entered in the table; subsequent names that
   * collide with it are not entered.
   */
  
  for (i=0; i<number; i++) {
    const char *thisName = names[i];
    int length = CoinStrlenAsInt(thisName);
    
    ipos = compute_hash(thisName, maxhash, length);
    if (hashThis[ipos].index == -1) {
      hashThis[ipos].index = i; // will be changed below
    }
  }
  
  /*
   * Now take care of the names that collided in the preceding loop,
   * by finding some other entry in the table for them.
   * Since there are as many entries in the table as there are names,
   * there must be room for them.
   * Also setting up hashNames.
   */

  int cnt_distinct = 0;
  
  iput = -1;
  for (i=0; i<number; i++) {
    const char *thisName = names[i];
    int length = CoinStrlenAsInt(thisName);
    
    ipos = compute_hash(thisName, maxhash, length);
    
    while (1) {
      COINColumnIndex j1 = hashThis[ipos].index;
      
      if(j1 == i) {

	// first occurence of thisName in the parameter "names"

	hashThis[ipos].index = cnt_distinct;
	hashNames[cnt_distinct] = CoinStrdup(thisName);
	cnt_distinct++;
	break;
      }
      else {

#ifdef LPIO_DEBUG
	if(j1 > i) {
	  char str[8192];
	  sprintf(str,"### ERROR: Hash table: j1: %d  i: %d\n", j1, i);
	  throw CoinError(str, "startHash", "CoinLpIO", __FILE__, __LINE__);
	}
#endif

	if (strcmp(thisName, hashNames[j1]) == 0) {

	  // thisName already entered

	  break;
	}
	else {
	  // Collision; check if thisName already entered

	  COINColumnIndex k = hashThis[ipos].next;

	  if (k == -1) {

	    // thisName not found; enter it

	    while (1) {
	      ++iput;
	      if (iput > maxhash) {
		char str[8192];
		sprintf(str,"### ERROR: Hash table: too many names\n");
		throw CoinError(str, "startHash", "CoinLpIO", __FILE__, __LINE__);
		break;
	      }
	      if (hashThis[iput].index == -1) {
		break;
	      }
	    }
	    hashThis[ipos].next = iput;
	    hashThis[iput].index = cnt_distinct;
	    hashNames[cnt_distinct] = CoinStrdup(thisName);
	    cnt_distinct++;
	    break;
	  } 
	  else {
	    ipos = k;

	    // continue the check with names in collision 

	  }
	}
      }
    }
  }

  numberHash_[section] = cnt_distinct;

} /* startHash */

/**************************************************************************/
//  stopHash.  Deletes hash storage
void
CoinLpIO::stopHash(int section)
{
  freePreviousNames(section);
  previous_names_[section] = names_[section];
  card_previous_names_[section] = numberHash_[section];

  delete[] hash_[section];
  hash_[section] = NULL;

  maxHash_[section] = 0;
  numberHash_[section] = 0;

  if(section == 0) {
    free(objName_);
    objName_ = NULL;
  }
} /* stopHash */

/**********************************************************************/
//  findHash.  -1 not found
COINColumnIndex
CoinLpIO::findHash(const char *name, int section) const
{
  COINColumnIndex found = -1;

  char ** names = names_[section];
  CoinHashLink * hashThis = hash_[section];
  COINColumnIndex maxhash = maxHash_[section];
  COINColumnIndex ipos;

  /* default if we don't find anything */
  if (!maxhash)
    return -1;

  int length = CoinStrlenAsInt(name);

  ipos = compute_hash(name, maxhash, length);
  while (1) {
    COINColumnIndex j1 = hashThis[ipos].index;

    if (j1 >= 0) {
      char *thisName2 = names[j1];

      if (strcmp (name, thisName2) != 0) {
	COINColumnIndex k = hashThis[ipos].next;

	if (k != -1)
	  ipos = k;
	else
	  break;
      } 
      else {
	found = j1;
	break;
      }
    } 
    else {
      found = -1;
      break;
    }
  }
  return found;
} /* findHash */

/*********************************************************************/
void
CoinLpIO::insertHash(const char *thisName, int section)
{

  int number = numberHash_[section];
  int maxhash = maxHash_[section];

  CoinHashLink * hashThis = hash_[section];
  char **hashNames = names_[section];

  int iput = -1;
  int length = CoinStrlenAsInt(thisName);

  int ipos = compute_hash(thisName, maxhash, length);

  while (1) {
    COINColumnIndex j1 = hashThis[ipos].index;
    
    if (j1 == -1) {
      hashThis[ipos].index = number;
      break;
    }
    else {
      char *thisName2 = hashNames[j1];
      
      if ( strcmp (thisName, thisName2) != 0 ) {
	COINColumnIndex k = hashThis[ipos].next;

	if (k == -1) {
	  while (1) {
	    ++iput;
	    if (iput == maxhash) {
	      char str[8192];
	      sprintf(str,"### ERROR: Hash table: too many names\n");
	      throw CoinError(str, "insertHash", "CoinLpIO", __FILE__, __LINE__);
	      break;
	    }
	    if (hashThis[iput].index == -1) {
	      break;
	    }
	  }
	  hashThis[ipos].next = iput;
	  hashThis[iput].index = number;
	  break;
	} 
	else {
	  ipos = k;
	  /* nothing worked - try it again */
	}
      }
    }
  }

  hashNames[number] = CoinStrdup(thisName);
  (numberHash_[section])++;

}
// Pass in Message handler (not deleted at end)
void 
CoinLpIO::passInMessageHandler(CoinMessageHandler * handler)
{
  if (defaultHandler_){ 
    delete handler_;
    handler = NULL; 
    }
  defaultHandler_=false;
  handler_=handler;
}
// Set language
void 
CoinLpIO::newLanguage(CoinMessages::Language language)
{
  messages_ = CoinMessage(language);
}

