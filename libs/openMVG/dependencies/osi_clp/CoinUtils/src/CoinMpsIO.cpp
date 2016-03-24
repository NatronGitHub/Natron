/* $Id: CoinMpsIO.cpp 1581 2013-04-06 12:48:50Z stefan $ */
// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "CoinUtilsConfig.h"

#include <cassert>
#include <cstdlib>
#include <cmath>
#include <cfloat>
#include <string>
#include <cstdio>
#include <iostream>

#include "CoinMpsIO.hpp"
#include "CoinMessage.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinModel.hpp"
#include "CoinSort.hpp"

//#############################################################################
// type - 0 normal, 1 INTEL IEEE, 2 other IEEE

namespace {

  const double fraction[]=
  {1.0,1.0e-1,1.0e-2,1.0e-3,1.0e-4,1.0e-5,1.0e-6,1.0e-7,1.0e-8,
   1.0e-9,1.0e-10,1.0e-11,1.0e-12,1.0e-13,1.0e-14,1.0e-15,1.0e-16,
   1.0e-17,1.0e-18,1.0e-19,1.0e-20,1.0e-21,1.0e-22,1.0e-23};

  const double exponent[]=
  {1.0e-9,1.0e-8,1.0e-7,1.0e-6,1.0e-5,1.0e-4,1.0e-3,1.0e-2,1.0e-1,
   1.0,1.0e1,1.0e2,1.0e3,1.0e4,1.0e5,1.0e6,1.0e7,1.0e8,1.0e9};

} // end file-local namespace
double CoinMpsCardReader::osi_strtod(char * ptr, char ** output, int type) 
{

  double value = 0.0;
  char * save = ptr;

  // take off leading white space
  while (*ptr==' '||*ptr=='\t')
    ptr++;
  if (!type) {
    double sign1=1.0;
    // do + or -
    if (*ptr=='-') {
      sign1=-1.0;
      ptr++;
    } else if (*ptr=='+') {
      ptr++;
    }
    // more white space
    while (*ptr==' '||*ptr=='\t')
      ptr++;
    char thisChar=0;
    while (value<1.0e30) {
      thisChar = *ptr;
      ptr++;
      if (thisChar>='0'&&thisChar<='9') 
	value = value*10.0+thisChar-'0';
      else
	break;
    }
    if (value<1.0e30) {
      if (thisChar=='.') {
	// do fraction
	double value2 = 0.0;
	int nfrac=0;
	while (nfrac<24) {
	  thisChar = *ptr;
	  ptr++;
	  if (thisChar>='0'&&thisChar<='9') {
	    value2 = value2*10.0+thisChar-'0';
	    nfrac++;
	  } else {
	    break;
	  }
	}
	if (nfrac<24) {
	  value += value2*fraction[nfrac];
	} else {
	  thisChar='x'; // force error
	}
      }
      if (thisChar=='e'||thisChar=='E') {
	// exponent
	int sign2=1;
	// do + or -
	if (*ptr=='-') {
	  sign2=-1;
	  ptr++;
	} else if (*ptr=='+') {
	  ptr++;
	}
	int value3 = 0;
	while (value3<1000) {
	  thisChar = *ptr;
	  ptr++;
	  if (thisChar>='0'&&thisChar<='9') {
	    value3 = value3*10+thisChar-'0';
	  } else {
	    break;
	  }
	}
	if (value3<300) {
	  value3 *= sign2; // power of 10
	  if (abs(value3)<10) {
	    // do most common by lookup (for accuracy?)
	    value *= exponent[value3+9];
	  } else {
	    value *= pow(10.0,value3);
	  }
	} else if (sign2<0.0) {
	  value = 0.0; // force zero
	} else {
	  value = COIN_DBL_MAX;
	}
      } 
      if (thisChar==0||thisChar=='\t'||thisChar==' ') {
	// okay
	*output=ptr;
      } else {
	value = osi_strtod(save,output);
	sign1=1.0;
      }
    } else {
      // bad value
      value = osi_strtod(save,output);
      sign1=1.0;
    }
    value *= sign1;
  } else {
    // ieee - 3 bytes go to 2
    assert (sizeof(double)==8*sizeof(char));
    assert (sizeof(unsigned short) == 2*sizeof(char));
    unsigned short shortValue[4];
    *output = ptr+12; // say okay
    if (type==1) {
      // INTEL
      for (int i=3;i>=0;i--) {
	int integerValue=0;
	char * three = reinterpret_cast<char *> (&integerValue);
	three[1]=ptr[0];
	three[2]=ptr[1];
	three[3]=ptr[2];
	unsigned short thisValue=0;
	// decode 6 bits at a time
	for (int j=2;j>=0;j--) {
	  thisValue = static_cast<unsigned short>(thisValue<<6);
	  char thisChar = ptr[j];
	  if (thisChar >= '0' && thisChar <= '0' + 9) {
	    thisValue = static_cast<unsigned short>(thisValue | (thisChar - '0'));
	  } else if (thisChar >= 'a' && thisChar <= 'a' + 25) {
	    thisValue = static_cast<unsigned short>(thisValue | (thisChar - 'a' + 10));
	  } else if (thisChar >= 'A' && thisChar <= 'A' + 25) {
	    thisValue = static_cast<unsigned short>(thisValue | (thisChar - 'A' + 36));
	  } else if (thisChar >= '*' && thisChar <= '*' + 1) {
	    thisValue = static_cast<unsigned short>(thisValue | (thisChar - '*' + 62));
	  } else {
	    // error 
	    *output=save;
	  }
	}
	ptr+=3;
	shortValue[i]=thisValue;
      }
    } else {
      // not INTEL
      for (int i=0;i<4;i++) {
	int integerValue=0;
	char * three = reinterpret_cast<char *> (&integerValue);
	three[1]=ptr[0];
	three[2]=ptr[1];
	three[3]=ptr[2];
	unsigned short thisValue=0;
	// decode 6 bits at a time
	for (int j=2;j>=0;j--) {
	  thisValue = static_cast<unsigned short>(thisValue<<6);
	  char thisChar = ptr[j];
	  if (thisChar >= '0' && thisChar <= '0' + 9) {
	    thisValue = static_cast<unsigned short>(thisValue | (thisChar - '0'));
	  } else if (thisChar >= 'a' && thisChar <= 'a' + 25) {
	    thisValue = static_cast<unsigned short>(thisValue | (thisChar - 'a' + 10));
	  } else if (thisChar >= 'A' && thisChar <= 'A' + 25) {
	    thisValue = static_cast<unsigned short>(thisValue | (thisChar - 'A' + 36));
	  } else if (thisChar >= '*' && thisChar <= '*' + 1) {
	    thisValue = static_cast<unsigned short>(thisValue | (thisChar - '*' + 62));
	  } else {
	    // error 
	    *output=save;
	  }
	}
	ptr+=3;
	shortValue[i]=thisValue;
      }
    }
    memcpy(&value,shortValue,sizeof(double));
  }
  return value;
}
// for strings 
double CoinMpsCardReader::osi_strtod(char * ptr, char ** output) 
{
  char * save = ptr;
  double value=-1.0e100;
  if (!stringsAllowed_) {
    *output=save;
  } else {
    // take off leading white space
    while (*ptr==' '||*ptr=='\t')
      ptr++;
    if (*ptr=='=') {
      strcpy(valueString_,ptr);
#define STRING_VALUE  -1.234567e-101
      value = STRING_VALUE;
      *output=ptr+strlen(ptr);
    } else {
      *output=save;
    }
  }
  return value;
}
//#############################################################################
// sections
const static char *section[] = {
  "", "NAME", "ROW", "COLUMN", "RHS", "RANGES", "BOUNDS", "ENDATA", " ","QSECTION", "CSECTION", 
  "QUADOBJ" , "SOS", "BASIS",
  " "
};

// what is allowed in each section - must line up with COINSectionType
const static COINMpsType startType[] = {
  COIN_UNKNOWN_MPS_TYPE, COIN_UNKNOWN_MPS_TYPE,
  COIN_N_ROW, COIN_BLANK_COLUMN,
  COIN_BLANK_COLUMN, COIN_BLANK_COLUMN,
  COIN_UP_BOUND, COIN_UNKNOWN_MPS_TYPE,
  COIN_UNKNOWN_MPS_TYPE,
  COIN_BLANK_COLUMN, COIN_BLANK_COLUMN, COIN_BLANK_COLUMN, COIN_S1_BOUND, 
  COIN_BS_BASIS, COIN_UNKNOWN_MPS_TYPE
};
const static COINMpsType endType[] = {
  COIN_UNKNOWN_MPS_TYPE, COIN_UNKNOWN_MPS_TYPE,
  COIN_BLANK_COLUMN, COIN_UNSET_BOUND,
  COIN_S1_COLUMN, COIN_S1_COLUMN,
  COIN_UNKNOWN_MPS_TYPE, COIN_UNKNOWN_MPS_TYPE,
  COIN_UNKNOWN_MPS_TYPE,
  COIN_BLANK_COLUMN, COIN_BLANK_COLUMN, COIN_BLANK_COLUMN, COIN_BS_BASIS,
  COIN_UNKNOWN_MPS_TYPE, COIN_UNKNOWN_MPS_TYPE
};
const static int allowedLength[] = {
  0, 0,
  1, 2,
  0, 0,
  2, 0,
  0, 0,
  0, 0,
  0, 2,
  0
};

// names of types
const static char *mpsTypes[] = {
  "N", "E", "L", "G",
  "  ", "S1", "S2", "S3", "  ", "  ", "  ",
  "  ", "UP", "FX", "LO", "FR", "MI", "PL", "BV", "UI", "LI", "SC",
  "X1", "X2", "BS", "XL", "XU", "LL", "UL", "  "
};

int CoinMpsCardReader::cleanCard()
{
  char * getit;
  getit = input_->gets ( card_, MAX_CARD_LENGTH);

  if ( getit ) {
    cardNumber_++;
    unsigned char * lastNonBlank = reinterpret_cast<unsigned char *> (card_-1);
    unsigned char * image = reinterpret_cast<unsigned char *> (card_);
    bool tabs=false;
    while ( *image != '\0' ) {
      if ( *image != '\t' && *image < ' ' ) {
	break;
      } else if ( *image != '\t' && *image != ' ') {
	lastNonBlank = image;
      } else if (*image == '\t') {
        tabs=true;
      }
      image++;
    }
    *(lastNonBlank+1)='\0';
    if (tabs&&section_ == COIN_BOUNDS_SECTION&&!freeFormat_&&eightChar_) {
      int length = static_cast<int>(lastNonBlank+1-
      				    reinterpret_cast<unsigned char *>(card_));
      assert (length<81);
      memcpy(card_+82,card_,length);
      int pos[]={1,4,14,24,1000};
      int put=0;
      int tab=0;
      for (int i=0;i<length;i++) {
        char look = card_[i+82];
        if (look!='\t') {
          card_[put++]=look;
        } else {
          // count on to next
          for (;tab<5;tab++) {
            if (put<pos[tab]) {
              while (put<pos[tab])
                card_[put++]= ' ';
              break;
            }
          }
        }
      }
      card_[put++]='\0';
    }
    return 0;
  } else {
    return 1;
  }
}

char *
CoinMpsCardReader::nextBlankOr ( char *image )
{
  char * saveImage=image;
  while ( 1 ) {
    if ( *image == ' ' || *image == '\t' ) {
      break;
    }
    if ( *image == '\0' )
      return NULL;
    image++;
  }
  // Allow for floating - or +.  Will fail if user has that as row name!!
  if (image-saveImage==1&&(*saveImage=='+'||*saveImage=='-')) {
    while ( *image == ' ' || *image == '\t' ) {
      image++;
    }
    image=nextBlankOr(image);
  }
  return image;
}

// Read to NAME card - return nonzero if bad
COINSectionType
CoinMpsCardReader::readToNextSection (  )
{
  bool found = false;

  while ( !found ) {
    // need new image

    if ( cleanCard() ) {
      section_ = COIN_EOF_SECTION;
      break;
    }
    if ( !strncmp ( card_, "NAME", 4 ) ||
		!strncmp( card_, "TIME", 4 ) ||
		!strncmp( card_, "BASIS", 5 ) ||
		!strncmp( card_, "STOCH", 5 ) ) {
      section_ = COIN_NAME_SECTION;
      char *next = card_ + 5;
      position_ = eol_ = card_+strlen(card_);

      handler_->message(COIN_MPS_LINE,messages_)<<cardNumber_
					       <<card_<<CoinMessageEol;
      while ( next < eol_ ) {
	if ( *next == ' ' || *next == '\t' ) {
	  next++;
	} else {
	  break;
	}
      }
      if ( next < eol_ ) {
	char *nextBlank = nextBlankOr ( next );
	char save;

	if ( nextBlank ) {
	  save = *nextBlank;
	  *nextBlank = '\0';
	  strcpy ( columnName_, next );
	  *nextBlank = save;
	  if ( strstr ( nextBlank, "FREEIEEE" ) ) {
	    freeFormat_ = true;
	    // see if intel
	    ieeeFormat_=1;
	    double value=1.0;
	    char x[8];
	    memcpy(x,&value,8);
	    if (x[0]==63) {
	      ieeeFormat_=2; // not intel
	    } else {
	      assert (x[0]==0);
	    }
	  } else if ( strstr ( nextBlank, "FREE" ) ) {
	    freeFormat_ = true;
	  } else if ( strstr ( nextBlank, "VALUES" ) ) {
	    // basis is always free - just use this to communicate back
	    freeFormat_ = true;
	  } else if ( strstr ( nextBlank, "IEEE" ) ) {
	    // see if intel
	    ieeeFormat_=1;
	    double value=1.0;
	    char x[8];
	    memcpy(x,&value,8);
	    if (x[0]==63) {
	      ieeeFormat_=2; // not intel
	    } else {
	      assert (x[0]==0);
	    }
	  }
	} else {
	  strcpy ( columnName_, next );
	}
      } else {
	strcpy ( columnName_, "no_name" );
      }
      break;
    } else if ( card_[0] != '*' && card_[0] != '#' ) {
      // not a comment
      int i;

      handler_->message(COIN_MPS_LINE,messages_)<<cardNumber_
					       <<card_<<CoinMessageEol;
      for ( i = COIN_ROW_SECTION; i < COIN_UNKNOWN_SECTION; i++ ) {
	if ( !strncmp ( card_, section[i], strlen ( section[i] ) ) ) {
	  break;
	}
      }
      position_ = card_;
      eol_ = card_;
      section_ = static_cast< COINSectionType > (i);
      break;
    }
  }
  return section_;
}

CoinMpsCardReader::CoinMpsCardReader (  CoinFileInput *input, 
					CoinMpsIO * reader)
{
  memset ( card_, 0, MAX_CARD_LENGTH );
  position_ = card_;
  eol_ = card_;
  mpsType_ = COIN_UNKNOWN_MPS_TYPE;
  memset ( rowName_, 0, COIN_MAX_FIELD_LENGTH );
  memset ( columnName_, 0, COIN_MAX_FIELD_LENGTH );
  value_ = 0.0;
  input_ = input;
  section_ = COIN_EOF_SECTION;
  cardNumber_ = 0;
  freeFormat_ = false;
  ieeeFormat_ = 0;
  eightChar_ = true;
  reader_ = reader;
  handler_ = reader_->messageHandler();
  messages_ = reader_->messages();
  memset ( valueString_, 0, COIN_MAX_FIELD_LENGTH );
  stringsAllowed_=false;
}
//  ~CoinMpsCardReader.  Destructor
CoinMpsCardReader::~CoinMpsCardReader (  )
{
  delete input_;
}

void
CoinMpsCardReader::strcpyAndCompress ( char *to, const char *from )
{
  int n = static_cast<int>(strlen(from));
  int i;
  int nto = 0;

  for ( i = 0; i < n; i++ ) {
    if ( from[i] != ' ' ) {
      to[nto++] = from[i];
    }
  }
  if ( !nto )
    to[nto++] = ' ';
  to[nto] = '\0';
}

//  nextField
COINSectionType
CoinMpsCardReader::nextField (  )
{
  mpsType_ = COIN_BLANK_COLUMN;
  // find next non blank character
  char *next = position_;

  while ( next != eol_ ) {
    if ( *next == ' ' || *next == '\t' ) {
      next++;
    } else {
      break;
    }
  }
  bool gotCard;

  if ( next == eol_ ) {
    gotCard = false;
  } else {
    gotCard = true;
  }
  while ( !gotCard ) {
    // need new image

    if ( cleanCard() ) {
      return COIN_EOF_SECTION;
    }
    if ( card_[0] == ' ' || card_[0] == '\0') {
      // not a section or comment
      position_ = card_;
      eol_ = card_ + strlen ( card_ );
      // get mps type and column name
      // scan to first non blank
      next = card_;
      while ( next != eol_ ) {
	if ( *next == ' ' || *next == '\t' ) {
	  next++;
	} else {
	  break;
	}
      }
      if ( next != eol_ ) {
	char *nextBlank = nextBlankOr ( next );
	int nchar;

	if ( nextBlank ) {
	  nchar = static_cast<int>(nextBlank - next);
	} else {
	  nchar = -1;
	}
	mpsType_ = COIN_BLANK_COLUMN;
	// special coding if RHS or RANGES, not free format and blanks
	if ( ( section_ != COIN_RHS_SECTION 
	       && section_ != COIN_RANGES_SECTION )
	     || freeFormat_ || strncmp ( card_ + 4, "        ", 8 ) ) {
	  // if columns section only look for first field if MARKER
	  if ( section_ == COIN_COLUMN_SECTION
	       && !strstr ( next, "'MARKER'" ) ) nchar = -1;
	  if (section_ == COIN_SOS_SECTION) {
	    if (!strncmp(card_," S1",3)) {
	      mpsType_ = COIN_S1_BOUND;
	      break;
	    } else if (!strncmp(card_," S2",3)) {
	      mpsType_ = COIN_S2_BOUND;
	      break;
	    }
	  }
	  if ( nchar == allowedLength[section_] ) {
	    //could be a type
	    int i;

	    for ( i = startType[section_]; i < endType[section_]; i++ ) {
	      if ( !strncmp ( next, mpsTypes[i], nchar ) ) {
		mpsType_ = static_cast<COINMpsType> (i);
		break;
	      }
	    }
	    if ( mpsType_ != COIN_BLANK_COLUMN ) {
	      //we know all we need so we can skip over
	      next = nextBlank;
	      while ( next != eol_ ) {
		if ( *next == ' ' || *next == '\t' ) {
		  next++;
		} else {
		  break;
		}
	      }
	      if ( next == eol_ ) {
		// error
		position_ = eol_;
		mpsType_ = COIN_UNKNOWN_MPS_TYPE;
	      } else {
		nextBlank = nextBlankOr ( next );
	      }
	    } else if (section_ == COIN_BOUNDS_SECTION) {
              // should have been something - but just fix LI problem
              // set to something illegal
              if (card_[0]==' '&&card_[3]==' '&&(card_[1]!=' '||card_[2]!=' ')) {
                mpsType_ = COIN_S3_COLUMN;
                //we know all we need so we can skip over
                next = nextBlank;
                while ( next != eol_ ) {
                  if ( *next == ' ' || *next == '\t' ) {
                    next++;
                  } else {
                    break;
                  }
                }
                if ( next == eol_ ) {
                  // error
                  position_ = eol_;
                  mpsType_ = COIN_UNKNOWN_MPS_TYPE;
                } else {
                  nextBlank = nextBlankOr ( next );
                }
              }
            }
	  }
	  if ( mpsType_ != COIN_UNKNOWN_MPS_TYPE ) {
	    // special coding if BOUND, not free format and blanks
	    if ( section_ != COIN_BOUNDS_SECTION ||
		 freeFormat_ || strncmp ( card_ + 4, "        ", 8 ) ) {
	      char save = '?';

	      if ( !freeFormat_ && eightChar_ && next == card_ + 4 ) {
		if ( eol_ - next >= 8 ) {
		  if ( *( next + 8 ) != ' ' && *( next + 8 ) != '\0' ) {
		    eightChar_ = false;
		  } else {
		    nextBlank = next + 8;
		  }
		  if (nextBlank) {
		    save = *nextBlank;
		    *nextBlank = '\0';
		  }
		} else {
		  nextBlank = NULL;
		}
	      } else {
		if ( nextBlank ) {
		  save = *nextBlank;
		  *nextBlank = '\0';
		}
	      }
	      strcpyAndCompress ( columnName_, next );
	      if ( nextBlank ) {
		*nextBlank = save;
		// on to next
		next = nextBlank;
	      } else {
		next = eol_;
	      }
	    } else {
	      // blank bounds name
	      strcpy ( columnName_, "        " );
	    }
	    while ( next != eol_ ) {
	      if ( *next == ' ' || *next == '\t' ) {
		next++;
	      } else {
		break;
	      }
	    }
	    if ( next == eol_ ) {
	      // error unless row section or conic section
	      position_ = eol_;
	      value_ = -1.0e100;
	      if ( section_ != COIN_ROW_SECTION && 
		   section_!= COIN_CONIC_SECTION)
		mpsType_ = COIN_UNKNOWN_MPS_TYPE;
	      else
		return section_;
	    } else {
	      nextBlank = nextBlankOr ( next );
	      //if (section_==COIN_CONIC_SECTION)
	    }
	    if ( section_ != COIN_ROW_SECTION ) {
	      char save = '?';

	      if ( !freeFormat_ && eightChar_ && next == card_ + 14 ) {
		if ( eol_ - next >= 8 ) {
		  if ( *( next + 8 ) != ' ' && *( next + 8 ) != '\0' ) {
		    eightChar_ = false;
		  } else {
		    nextBlank = next + 8;
		  }
		  save = *nextBlank;
		  *nextBlank = '\0';
		} else {
		  nextBlank = NULL;
		}
	      } else {
		if ( nextBlank ) {
		  save = *nextBlank;
		  *nextBlank = '\0';
		}
	      }
	      strcpyAndCompress ( rowName_, next );
	      if ( nextBlank ) {
		*nextBlank = save;
		// on to next
		next = nextBlank;
	      } else {
		next = eol_;
	      }
	      while ( next != eol_ ) {
		if ( *next == ' ' || *next == '\t' ) {
		  next++;
		} else {
		  break;
		}
	      }
	      // special coding for markers
	      if ( section_ == COIN_COLUMN_SECTION &&
		   !strncmp ( rowName_, "'MARKER'", 8 ) && next != eol_ ) {
		if ( !strncmp ( next, "'INTORG'", 8 ) ) {
		  mpsType_ = COIN_INTORG;
		} else if ( !strncmp ( next, "'INTEND'", 8 ) ) {
		  mpsType_ = COIN_INTEND;
		} else if ( !strncmp ( next, "'SOSORG'", 8 ) ) {
		  if ( mpsType_ == COIN_BLANK_COLUMN )
		    mpsType_ = COIN_S1_COLUMN;
		} else if ( !strncmp ( next, "'SOSEND'", 8 ) ) {
		  mpsType_ = COIN_SOSEND;
		} else {
		  mpsType_ = COIN_UNKNOWN_MPS_TYPE;
		}
		position_ = eol_;
		return section_;
	      }
	      if ( next == eol_ ) {
		// error unless bounds or basis
		position_ = eol_;
		if ( section_ != COIN_BOUNDS_SECTION ) {
		  if ( section_ != COIN_BASIS_SECTION ) 
		    mpsType_ = COIN_UNKNOWN_MPS_TYPE;
		  value_ = -1.0e100;
		} else {
		  value_ = 0.0;
		}
	      } else {
		nextBlank = nextBlankOr ( next );
		if ( nextBlank ) {
		  save = *nextBlank;
		  *nextBlank = '\0';
		}
		char * after;
		value_ = osi_strtod(next,&after,ieeeFormat_);
		// see if error
		if (after>next) {
                  if ( nextBlank ) {
                    *nextBlank = save;
                    position_ = nextBlank;
                  } else {
                    position_ = eol_;
                  }
                } else {
                  // error
                  position_ = eol_;
                  mpsType_ = COIN_UNKNOWN_MPS_TYPE;
		  value_ = -1.0e100;
                }
	      }
	    }
	  }
	} else {
	  //blank name in RHS or RANGES
	  strcpy ( columnName_, "        " );
	  char save = '?';

	  if ( !freeFormat_ && eightChar_ && next == card_ + 14 ) {
	    if ( eol_ - next >= 8 ) {
	      if ( *( next + 8 ) != ' ' && *( next + 8 ) != '\0' ) {
		eightChar_ = false;
	      } else {
		nextBlank = next + 8;
	      }
	      save = *nextBlank;
	      *nextBlank = '\0';
	    } else {
	      nextBlank = NULL;
	    }
	  } else {
	    if ( nextBlank ) {
	      save = *nextBlank;
	      *nextBlank = '\0';
	    }
	  }
	  strcpyAndCompress ( rowName_, next );
	  if ( nextBlank ) {
	    *nextBlank = save;
	    // on to next
	    next = nextBlank;
	  } else {
	    next = eol_;
	  }
	  while ( next != eol_ ) {
	    if ( *next == ' ' || *next == '\t' ) {
	      next++;
	    } else {
	      break;
	    }
	  }
	  if ( next == eol_ ) {
	    // error 
	    position_ = eol_;
	    value_ = -1.0e100;
	    mpsType_ = COIN_UNKNOWN_MPS_TYPE;
	  } else {
	    nextBlank = nextBlankOr ( next );
	    value_ = -1.0e100;
	    if ( nextBlank ) {
	      save = *nextBlank;
	      *nextBlank = '\0';
	    }
	    char * after;
	    value_ = osi_strtod(next,&after,ieeeFormat_);
	    // see if error
	    if (after>next) {
              if ( nextBlank ) {
                *nextBlank = save;
                position_ = nextBlank;
              } else {
                position_ = eol_;
              }
            } else {
              // error
              position_ = eol_;
              mpsType_ = COIN_UNKNOWN_MPS_TYPE;
              value_ = -1.0e100;
            }
	  }
	}
      } else {
	// blank
	continue;
      }
      return section_;
    } else if ( card_[0] != '*' ) {
      // not a comment
      int i;

      handler_->message(COIN_MPS_LINE,messages_)<<cardNumber_
					       <<card_<<CoinMessageEol;
      for ( i = COIN_ROW_SECTION; i < COIN_UNKNOWN_SECTION; i++ ) {
	if ( !strncmp ( card_, section[i], strlen ( section[i] ) ) ) {
	  break;
	}
      }
      position_ = card_;
      eol_ = card_;
      section_ = static_cast<COINSectionType> (i);
      return section_;
    } else {
      // comment
    }
  }
  // we only get here for second field (we could even allow more???)
  {
    char save = '?';
    char *nextBlank = nextBlankOr ( next );

    if ( !freeFormat_ && eightChar_ && next == card_ + 39 ) {
      if ( eol_ - next >= 8 ) {
	if ( *( next + 8 ) != ' ' && *( next + 8 ) != '\0' ) {
	  eightChar_ = false;
	} else {
	  nextBlank = next + 8;
	}
	save = *nextBlank;
	*nextBlank = '\0';
      } else {
	nextBlank = NULL;
      }
    } else {
      if ( nextBlank ) {
	save = *nextBlank;
	*nextBlank = '\0';
      }
    }
    strcpyAndCompress ( rowName_, next );
    // on to next
    if ( nextBlank ) {
      *nextBlank = save;
      next = nextBlank;
    } else {
      next = eol_;
    }
    while ( next != eol_ ) {
      if ( *next == ' ' || *next == '\t' ) {
	next++;
      } else {
	break;
      }
    }
    if ( next == eol_ && section_ != COIN_SOS_SECTION) {
      // error
      position_ = eol_;
      mpsType_ = COIN_UNKNOWN_MPS_TYPE;
    } else {
      nextBlank = nextBlankOr ( next );
    }
    if ( nextBlank ) {
      save = *nextBlank;
      *nextBlank = '\0';
    }
    //value_ = -1.0e100;
    char * after;
    value_ = osi_strtod(next,&after,ieeeFormat_);
    // see if error
    if (after>next) {
      if ( nextBlank ) {
        *nextBlank = save;
        position_ = nextBlank;
      } else {
        position_ = eol_;
      }
    } else {
      // error
      position_ = eol_;
      if (mpsType_!=COIN_S1_BOUND&&mpsType_!=COIN_S2_BOUND)
	mpsType_ = COIN_UNKNOWN_MPS_TYPE;
      value_ = -1.0e100;
    }
  }
  return section_;
}
static char *
nextNonBlank ( char *image )
{
  while ( 1 ) {
    if ( *image != ' ' && *image != '\t' ) 
      break;
    else
      image++;
  }
  if ( *image == '\0' )
    image=NULL;
  return image;
}
/** Gets next field for .gms file and returns type.
    -1 - EOF
    0 - what we expected (and processed so pointer moves past)
    1 - not what we expected
    2 - equation type when expecting value name pair
    leading blanks always ignored
    input types 
    0 - anything - stops on non blank card
    1 - name (in columnname)
    2 - value
    3 - value name pair
    4 - equation type
    5 - ;
*/
int 
CoinMpsCardReader::nextGmsField ( int expectedType )
{
  int returnCode=-1;
  bool good=false;
  switch(expectedType) {
  case 0:
    // 0 - May get * in first column or anything
    if ( cleanCard())
      return -1;
    while(!strlen(card_)) {
      if ( cleanCard())
	return -1;
    }
    eol_ = card_+strlen(card_);
    position_=card_;
    returnCode=0;
    break;
  case 1:
    // 1 - expect name
    while (!good) {
      position_ = nextNonBlank(position_);
      if (position_==NULL) {
	if ( cleanCard())
	  return -1;
	eol_ = card_+strlen(card_);
	position_=card_;
      } else {
	good=true;
	char nextChar =*position_;
	if ((nextChar>='a'&&nextChar<='z')||
	    (nextChar>='A'&&nextChar<='Z')) {
          returnCode=0;
          char * next=position_;
          while (*next!=','&&*next!=';'&&*next!='='&&*next!=' '
                 &&*next!='\t'&&*next!='-'&&*next!='+'&&*next>=32)
            next++;
          if (next) {
            int length = static_cast<int>(next-position_);
            strncpy(columnName_,position_,length);
            columnName_[length]='\0';
          } else {
            strcpy(columnName_,position_);
            next=eol_;
          }
          position_=next;
        } else {
          returnCode=1;
        }
      }
    }
    break;
  case 2:
    // 2 - expect value
    while (!good) {
      position_ = nextNonBlank(position_);
      if (position_==NULL) {
	if ( cleanCard())
	  return -1;
	eol_ = card_+strlen(card_);
	position_=card_;
      } else {
	good=true;
	char nextChar =*position_;
	if ((nextChar>='0'&&nextChar<='9')||nextChar=='+'||nextChar=='-') {
          returnCode=0;
          char * next=position_;
          while (*next!=','&&*next!=';'&&*next!='='&&*next!=' '
                 &&*next!='\t'&&*next>=32)
            next++;
          if (next) {
            int length = static_cast<int>(next-position_);
            strncpy(rowName_,position_,length);
            rowName_[length]='\0';
          } else {
            strcpy(rowName_,position_);
            next=eol_;
          }
          value_=-1.0e100;
          sscanf(rowName_,"%lg",&value_);
          position_=next;
        } else {
          returnCode=1;
        }
      }
    }
    break;
  case 3:
    // 3 - expect value name pair
    while (!good) {
      position_ = nextNonBlank(position_);
      char * savePosition = position_;
      if (position_==NULL) {
	if ( cleanCard())
	  return -1;
	eol_ = card_+strlen(card_);
	position_=card_;
        savePosition = position_;
      } else {
	good=true;
        value_=1.0;
	char nextChar =*position_;
        returnCode=0;
	if ((nextChar>='0'&&nextChar<='9')||nextChar=='+'||nextChar=='-') {
          char * next;
          int put=0;
          if (nextChar=='+'||nextChar=='-') {
            rowName_[0]=nextChar;
            put=1;
            next=position_+1;
            while (*next==' '||*next=='\t')
              next++;
            if ((*next>='a'&&*next<='z')||
                (*next>='A'&&*next<='Z')) {
              // name - set value
              if (nextChar=='+')
                value_=1.0;
              else
                value_=-1.0;
              position_=next;
            } else if ((*next>='0'&&*next<='9')||*next=='+'||*next=='-') {
              rowName_[put++]=*next;
              next++;
              while (*next!=' '&&*next!='\t'&&*next!='*') {
                rowName_[put++]=*next;
                next++;
              }
              assert (*next=='*');
              next ++;
              rowName_[put]='\0';
              value_=-1.0e100;
              sscanf(rowName_,"%lg",&value_);
              position_=next;
            } else {
              returnCode=1;
            }
          } else {
            // number
            char * next = nextBlankOr(position_);
            // but could be *
            char * next2 = strchr(position_,'*');
            if (next2&&next2-position_<next-position_) {
              next=next2;
            }
            int length = static_cast<int>(next-position_);
            strncpy(rowName_,position_,length);
            rowName_[length]='\0';
            value_=-1.0e100;
            sscanf(rowName_,"%lg",&value_);
            position_=next;
          }
	} else if ((nextChar>='a'&&nextChar<='z')||
                   (nextChar>='A'&&nextChar<='Z')) {
          // name so take value as 1.0
        } else if (nextChar=='=') {
          returnCode=2;
          position_=savePosition;
        } else {
          returnCode=1;
          position_=savePosition;
        }
        if ((*position_)=='*')
          position_++;
        position_= nextNonBlank(position_);
        if (!returnCode) {
          char nextChar =*position_;
          if ((nextChar>='a'&&nextChar<='z')||
              (nextChar>='A'&&nextChar<='Z')) {
            char * next = nextBlankOr(position_);
            if (next) {
              int length = static_cast<int>(next-position_);
              strncpy(columnName_,position_,length);
              columnName_[length]='\0';
            } else {
              strcpy(columnName_,position_);
              next=eol_;
            }
            position_=next;
          } else {
            returnCode=1;
            position_=savePosition;
          }
        }
      }
    }
    break;
  case 4:
    // 4 - expect equation type
    while (!good) {
      position_ = nextNonBlank(position_);
      if (position_==NULL) {
	if ( cleanCard())
	  return -1;
	eol_ = card_+strlen(card_);
	position_=card_;
      } else {
	good=true;
	char nextChar =*position_;
	if (nextChar=='=') {
          returnCode=0;
          char * next = nextBlankOr(position_);
          int length = static_cast<int>(next-position_);
          strncpy(rowName_,position_,length);
          rowName_[length]='\0';
          position_=next;
        } else {
          returnCode=1;
        }
      }
    }
    break;
  case 5:
    // 5 - ; expected
    while (!good) {
      position_ = nextNonBlank(position_);
      if (position_==NULL) {
	if ( cleanCard())
	  return -1;
	eol_ = card_+strlen(card_);
	position_=card_;
      } else {
	good=true;
	char nextChar =*position_;
	if (nextChar==';') {
          returnCode=0;
          char * next = nextBlankOr(position_);
          if (!next)
            next=eol_;
          position_=next;
        } else {
          returnCode=1;
        }
      }
    }
    break;
  }
  return returnCode;
}

//#############################################################################

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

int hash ( const char *name, int maxsiz, int length )
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

// Define below if you are reading a Cnnnnnn file 
// Will not do row names (for electricfence)
//#define NONAMES
#ifndef NONAMES
//  startHash.  Creates hash list for names
void
CoinMpsIO::startHash ( char **names, const COINColumnIndex number , int section )
{
  names_[section] = names;
  numberHash_[section] = number;
  startHash(section);
}
void
CoinMpsIO::startHash ( int section ) const
{
  char ** names = names_[section];
  COINColumnIndex number = numberHash_[section];
  COINColumnIndex i;
  COINColumnIndex maxhash = 4 * number;
  COINColumnIndex ipos, iput;

  //hash_=(CoinHashLink *) malloc(maxhash*sizeof(CoinHashLink));
  hash_[section] = new CoinHashLink[maxhash];
  
  CoinHashLink * hashThis = hash_[section];

  for ( i = 0; i < maxhash; i++ ) {
    hashThis[i].index = -1;
    hashThis[i].next = -1;
  }

  /*
   * Initialize the hash table.  Only the index of the first name that
   * hashes to a value is entered in the table; subsequent names that
   * collide with it are not entered.
   */
  for ( i = 0; i < number; ++i ) {
    char *thisName = names[i];
    int length = static_cast<int>(strlen(thisName));

    ipos = hash ( thisName, maxhash, length );
    if ( hashThis[ipos].index == -1 ) {
      hashThis[ipos].index = i;
    }
  }

  /*
   * Now take care of the names that collided in the preceding loop,
   * by finding some other entry in the table for them.
   * Since there are as many entries in the table as there are names,
   * there must be room for them.
   */
  iput = -1;
  for ( i = 0; i < number; ++i ) {
    char *thisName = names[i];
    int length = static_cast<int>(strlen(thisName));

    ipos = hash ( thisName, maxhash, length );

    while ( 1 ) {
      COINColumnIndex j1 = hashThis[ipos].index;

      if ( j1 == i )
	break;
      else {
	char *thisName2 = names[j1];

	if ( strcmp ( thisName, thisName2 ) == 0 ) {
	  printf ( "** duplicate name %s\n", names[i] );
	  break;
	} else {
	  COINColumnIndex k = hashThis[ipos].next;

	  if ( k == -1 ) {
	    while ( 1 ) {
	      ++iput;
	      if ( iput > number ) {
		printf ( "** too many names\n" );
		break;
	      }
	      if ( hashThis[iput].index == -1 ) {
		break;
	      }
	    }
	    hashThis[ipos].next = iput;
	    hashThis[iput].index = i;
	    break;
	  } else {
	    ipos = k;
	    /* nothing worked - try it again */
	  }
	}
      }
    }
  }
}

//  stopHash.  Deletes hash storage
void
CoinMpsIO::stopHash ( int section )
{
  delete [] hash_[section];
  hash_[section] = NULL;
}

//  findHash.  -1 not found
COINColumnIndex
CoinMpsIO::findHash ( const char *name , int section ) const
{
  COINColumnIndex found = -1;

  char ** names = names_[section];
  CoinHashLink * hashThis = hash_[section];
  COINColumnIndex maxhash = 4 * numberHash_[section];
  COINColumnIndex ipos;

  /* default if we don't find anything */
  if ( !maxhash )
    return -1;
  int length = static_cast<int>(strlen(name));

  ipos = hash ( name, maxhash, length );
  while ( 1 ) {
    COINColumnIndex j1 = hashThis[ipos].index;

    if ( j1 >= 0 ) {
      char *thisName2 = names[j1];

      if ( strcmp ( name, thisName2 ) != 0 ) {
	COINColumnIndex k = hashThis[ipos].next;

	if ( k != -1 )
	  ipos = k;
	else
	  break;
      } else {
	found = j1;
	break;
      }
    } else {
      found = -1;
      break;
    }
  }
  return found;
}
#else
// Version when we know images are C/Rnnnnnn
//  startHash.  Creates hash list for names
void
CoinMpsIO::startHash ( char **names, const COINColumnIndex number , int section )
{
  numberHash_[section] = number;
  names_[section] = names;
}
void
CoinMpsIO::startHash ( int section ) const
{
}

//  stopHash.  Deletes hash storage
void
CoinMpsIO::stopHash ( int section )
{
}

//  findHash.  -1 not found
COINColumnIndex
CoinMpsIO::findHash ( const char *name , int section ) const
{
  COINColumnIndex found = atoi(name+1);
  if (!strcmp(name,"OBJROW"))
    found = numberHash_[section]-1;
  return found;
}
#endif
//------------------------------------------------------------------
// Get value for infinity
//------------------------------------------------------------------
double CoinMpsIO::getInfinity() const
{
  return infinity_;
}
//------------------------------------------------------------------
// Set value for infinity
//------------------------------------------------------------------
void CoinMpsIO::setInfinity(double value) 
{
  if ( value >= 1.020 ) {
    infinity_ = value;
  } else {
    handler_->message(COIN_MPS_ILLEGAL,messages_)<<"infinity"
						<<value
						<<CoinMessageEol;
  }

}
// Set file name
void CoinMpsIO::setFileName(const char * name)
{
  free(fileName_);
  fileName_=CoinStrdup(name);
}
// Get file name
const char * CoinMpsIO::getFileName() const
{
  return fileName_;
}
// Deal with filename - +1 if new, 0 if same as before, -1 if error
int
CoinMpsIO::dealWithFileName(const char * filename,  const char * extension,
		       CoinFileInput * & input)
{
  if (input != 0) {
    delete input;
    input = 0;
  }

  int goodFile=0;

  if (!fileName_||(filename!=NULL&&strcmp(filename,fileName_))) {
    if (filename==NULL) {
      handler_->message(COIN_MPS_FILE,messages_)<<"NULL"
						<<CoinMessageEol;
      return -1;
    }
    goodFile=-1;
    // looks new name
    char newName[400];
    if (strcmp(filename,"stdin")&&strcmp(filename,"-")) {
      if (extension&&strlen(extension)) {
	// There was an extension - but see if user gave .xxx
	int i = static_cast<int>(strlen(filename))-1;
	strcpy(newName,filename);
	bool foundDot=false; 
	for (;i>=0;i--) {
	  char character = filename[i];
	  if (character=='/'||character=='\\') {
	    break;
	  } else if (character=='.') {
	    foundDot=true;
	    break;
	  }
	}
	if (!foundDot) {
	  strcat(newName,".");
	  strcat(newName,extension);
	}
      } else {
	// no extension
	strcpy(newName,filename);
      }
    } else {
      strcpy(newName,"stdin");    
    }
    // See if new name
    if (fileName_&&!strcmp(newName,fileName_)) {
      // old name
      return 0;
    } else {
      // new file
      free(fileName_);
      fileName_=CoinStrdup(newName);    
      if (strcmp(fileName_,"stdin")) {

	// be clever with extensions here
	std::string fname = fileName_;
	bool readable = fileCoinReadable(fname);
	if (!readable)
	  goodFile = -1;
	else
	  {
	    input = CoinFileInput::create (fname);
	    goodFile = 1;
	  }
      } else {
        // only plain file at present
        input = CoinFileInput::create ("stdin");
        goodFile = 1;
      }
    }
  } else {
    // same as before
    // reset section ?
    goodFile=0;
  }
  if (goodFile<0) 
    handler_->message(COIN_MPS_FILE,messages_)<<fileName_
					      <<CoinMessageEol;
  return goodFile;
}
/* objective offset - this is RHS entry for objective row */
double CoinMpsIO::objectiveOffset() const
{
  return objectiveOffset_;
}
/*
  Prior to June 2007, this was set to 1e30. But that causes problems in
  some of the cut generators --- they need to see finite infinity in order
  to work properly.
*/
#define MAX_INTEGER COIN_DBL_MAX
// Sets default upper bound for integer variables
void CoinMpsIO::setDefaultBound(int value)
{
  if ( value >= 1 && value <=MAX_INTEGER ) {
    defaultBound_ = value;
  } else {
    handler_->message(COIN_MPS_ILLEGAL,messages_)<<"default integer bound"
						<<value
						<<CoinMessageEol;
  }
}
// gets default upper bound for integer variables
int CoinMpsIO::getDefaultBound() const
{
  return defaultBound_;
}
//------------------------------------------------------------------
// Read mps files
//------------------------------------------------------------------
int CoinMpsIO::readMps(const char * filename,  const char * extension)
{
  // Deal with filename - +1 if new, 0 if same as before, -1 if error

  CoinFileInput *input = 0;
  int returnCode = dealWithFileName(filename,extension,input);
  if (returnCode<0) {
    return -1;
  } else if (returnCode>0) {
    delete cardReader_;
    cardReader_ = new CoinMpsCardReader ( input, this);
  }
  if (!extension||(strcmp(extension,"gms")&&!strstr(filename,".gms"))) {
    return readMps();
  } else {
    int numberSets=0;
    CoinSet ** sets=NULL;
    int returnCode = readGms(numberSets,sets);
    for (int i=0;i<numberSets;i++)
      delete sets[i];
    delete [] sets;
    return returnCode;
  }
}
int CoinMpsIO::readMps(const char * filename,  const char * extension,
		       int & numberSets,CoinSet ** &sets)
{
  // Deal with filename - +1 if new, 0 if same as before, -1 if error
  CoinFileInput *input = 0;
  int returnCode = dealWithFileName(filename,extension,input);
  if (returnCode<0) {
    return -1;
  } else if (returnCode>0) {
    delete cardReader_;
    cardReader_ = new CoinMpsCardReader ( input, this);
  }
  return readMps(numberSets,sets);
}
int CoinMpsIO::readMps()
{
  int numberSets=0;
  CoinSet ** sets=NULL;
  int returnCode = readMps(numberSets,sets);
  for (int i=0;i<numberSets;i++)
    delete sets[i];
  delete [] sets;
  return returnCode;
}
int CoinMpsIO::readMps(int & numberSets,CoinSet ** &sets)
{
  bool ifmps;

  cardReader_->readToNextSection();

  if ( cardReader_->whichSection (  ) == COIN_NAME_SECTION ) {
    ifmps = true;
    // save name of section
    free(problemName_);
    problemName_=CoinStrdup(cardReader_->columnName());
  } else if ( cardReader_->whichSection (  ) == COIN_UNKNOWN_SECTION ) {
    handler_->message(COIN_MPS_BADFILE1,messages_)<<cardReader_->card()
						  <<1
						 <<fileName_
						  <<CoinMessageEol;

    if (cardReader_->fileInput()->getReadType()!="plain") 
      handler_->message(COIN_MPS_BADFILE2,messages_)
        <<cardReader_->fileInput()->getReadType()
        <<CoinMessageEol;

    return -2;
  } else if ( cardReader_->whichSection (  ) != COIN_EOF_SECTION ) {
    // save name of section
    free(problemName_);
    problemName_=CoinStrdup(cardReader_->card());
    ifmps = false;
  } else {
    handler_->message(COIN_MPS_EOF,messages_)<<fileName_
					    <<CoinMessageEol;
    return -3;
  }
  CoinBigIndex *start;
  COINRowIndex *row;
  double *element;
  objectiveOffset_ = 0.0;

  int numberErrors = 0;
  int i;
  if ( ifmps ) {
    // mps file - always read in free format
    bool gotNrow = false;
    // allow strings ?
    if (allowStringElements_)
      cardReader_->setStringsAllowed();

    //get ROWS
    cardReader_->nextField (  ) ;
    // Fudge for what ever code has OBJSENSE
    if (!strncmp(cardReader_->card(),"OBJSENSE",8)) {
      cardReader_->nextField();
      int i;
      const char * thisCard = cardReader_->card();
      int direction = 0;
      for (i=0;i<20;i++) {
	if (thisCard[i]!=' ') {
	  if (!strncmp(thisCard+i,"MAX",3))
	    direction=-1;
	  else if (!strncmp(thisCard+i,"MIN",3))
	    direction=1;
	  break;
	}
      }
      if (!direction)
	printf("No MAX/MIN found after OBJSENSE\n");
      else 
	printf("%s found after OBJSENSE - Coin ignores\n",
	       (direction>0 ? "MIN" : "MAX"));
      cardReader_->nextField();
    }
    if ( cardReader_->whichSection (  ) != COIN_ROW_SECTION ) {
      handler_->message(COIN_MPS_BADIMAGE,messages_)<<cardReader_->cardNumber()
						    <<cardReader_->card()
						    <<CoinMessageEol;
      handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
      return numberErrors+100000;
    }
    //use malloc etc as I don't know how to do realloc in C++
    numberRows_ = 0;
    numberColumns_ = 0;
    numberElements_ = 0;
    COINRowIndex maxRows = 1000;
    COINMpsType *rowType =

      reinterpret_cast< COINMpsType *> (malloc ( maxRows * sizeof ( COINMpsType )));
    char **rowName = reinterpret_cast<char **> (malloc ( maxRows * sizeof ( char * )));

    // for discarded free rows
    COINRowIndex maxFreeRows = 100;
    COINRowIndex numberOtherFreeRows = 0;
    char **freeRowName =

      reinterpret_cast<char **> (malloc ( maxFreeRows * sizeof ( char * )));
    while ( cardReader_->nextField (  ) == COIN_ROW_SECTION ) {
      switch ( cardReader_->mpsType (  ) ) {
      case COIN_N_ROW:
	if ( !gotNrow ) {
	  gotNrow = true;
	  // save name of section
	  free(objectiveName_);
	  objectiveName_=CoinStrdup(cardReader_->columnName());
	} else {
	  // add to discard list
	  if ( numberOtherFreeRows == maxFreeRows ) {
	    maxFreeRows = ( 3 * maxFreeRows ) / 2 + 100;
	    freeRowName =
	      reinterpret_cast<char **> (realloc ( freeRowName,
					      maxFreeRows * sizeof ( char * )));
	  }
	  freeRowName[numberOtherFreeRows] =
	    CoinStrdup ( cardReader_->columnName (  ) );
	  numberOtherFreeRows++;
	}
	break;
      case COIN_E_ROW:
      case COIN_L_ROW:
      case COIN_G_ROW:
	if ( numberRows_ == maxRows ) {
	  maxRows = ( 3 * maxRows ) / 2 + 1000;
	  rowType =
	    reinterpret_cast<COINMpsType *> (realloc ( rowType,
						       maxRows * sizeof ( COINMpsType )));
	  rowName =

	    reinterpret_cast<char **> (realloc ( rowName, maxRows * sizeof ( char * )));
	}
	rowType[numberRows_] = cardReader_->mpsType (  );
#ifndef NONAMES
	rowName[numberRows_] = CoinStrdup ( cardReader_->columnName (  ) );
#endif
	numberRows_++;
	break;
      default:
	numberErrors++;
	if ( numberErrors < 100 ) {
	  handler_->message(COIN_MPS_BADIMAGE,messages_)<<cardReader_->cardNumber()
						       <<cardReader_->card()
						       <<CoinMessageEol;
	} else if (numberErrors > 100000) {
	  handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	  return numberErrors;
	}
      }
    }
    if ( cardReader_->whichSection (  ) != COIN_COLUMN_SECTION ) {
      handler_->message(COIN_MPS_BADIMAGE,messages_)<<cardReader_->cardNumber()
						    <<cardReader_->card()
						    <<CoinMessageEol;
      handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
      return numberErrors+100000;
    }
    //assert ( gotNrow );
    if (numberRows_)
      rowType =
	reinterpret_cast<COINMpsType *> (realloc ( rowType,
						   numberRows_ * sizeof ( COINMpsType )));
    else
      rowType =
	reinterpret_cast<COINMpsType *> (realloc ( rowType,sizeof ( COINMpsType )));
    // put objective and other free rows at end
    rowName =
      reinterpret_cast<char **> (realloc ( rowName,
			    ( numberRows_ + 1 +

			      numberOtherFreeRows ) * sizeof ( char * )));
#ifndef NONAMES
    rowName[numberRows_] = CoinStrdup(objectiveName_);
    memcpy ( rowName + numberRows_ + 1, freeRowName,
	     numberOtherFreeRows * sizeof ( char * ) );
    // now we can get rid of this array
    free(freeRowName);
#else
    memset(rowName,0,(numberRows_+1)*sizeof(char **));
#endif

    startHash ( rowName, numberRows_ + 1 + numberOtherFreeRows , 0 );
    COINColumnIndex maxColumns = 1000 + numberRows_ / 5;
    CoinBigIndex maxElements = 5000 + numberRows_ / 2;
    COINMpsType *columnType = reinterpret_cast<COINMpsType *>
      (malloc ( maxColumns * sizeof ( COINMpsType )));
    char **columnName = reinterpret_cast<char **> (malloc ( maxColumns * sizeof ( char * )));

    objective_ = reinterpret_cast<double *> (malloc ( maxColumns * sizeof ( double )));
    start = reinterpret_cast<CoinBigIndex *>
      (malloc ( ( maxColumns + 1 ) * sizeof ( CoinBigIndex )));
    row = reinterpret_cast<COINRowIndex *>
      (malloc ( maxElements * sizeof ( COINRowIndex )));
    element =
      reinterpret_cast<double *> (malloc ( maxElements * sizeof ( double )));
    // for duplicates
    CoinBigIndex *rowUsed = new CoinBigIndex[numberRows_];

    for (i=0;i<numberRows_;i++) {
      rowUsed[i]=-1;
    }
    bool objUsed = false;

    numberElements_ = 0;
    char lastColumn[200];

    memset ( lastColumn, '\0', 200 );
    COINColumnIndex column = -1;
    bool inIntegerSet = false;
    COINColumnIndex numberIntegers = 0;

    while ( cardReader_->nextField (  ) == COIN_COLUMN_SECTION ) {
      switch ( cardReader_->mpsType (  ) ) {
      case COIN_BLANK_COLUMN:
	if ( strcmp ( lastColumn, cardReader_->columnName (  ) ) ) {
	  // new column

	  // reset old column and take out tiny
	  if ( numberColumns_ ) {
	    objUsed = false;
	    CoinBigIndex i;
	    CoinBigIndex k = start[column];

	    for ( i = k; i < numberElements_; i++ ) {
	      COINRowIndex irow = row[i];
#if 0
	      if ( fabs ( element[i] ) > smallElement_ ) {
		element[k++] = element[i];
	      }
#endif
	      rowUsed[irow] = -1;
	    }
	    //numberElements_ = k;
	  }
	  column = numberColumns_;
	  if ( numberColumns_ == maxColumns ) {
	    maxColumns = ( 3 * maxColumns ) / 2 + 1000;
	    columnType = reinterpret_cast<COINMpsType *>
	      (realloc ( columnType, maxColumns * sizeof ( COINMpsType )));
	    columnName = reinterpret_cast<char **>
	      (realloc ( columnName, maxColumns * sizeof ( char * )));

	    objective_ = reinterpret_cast<double *>
	      (realloc ( objective_, maxColumns * sizeof ( double )));
	    start = reinterpret_cast<CoinBigIndex *>
	      (realloc ( start,
			 ( maxColumns + 1 ) * sizeof ( CoinBigIndex )));
	  }
	  if ( !inIntegerSet ) {
	    columnType[column] = COIN_UNSET_BOUND;
	  } else {
	    columnType[column] = COIN_INTORG;
	    numberIntegers++;
	  }
#ifndef NONAMES
	  columnName[column] = CoinStrdup ( cardReader_->columnName (  ) );
#else
          columnName[column]=NULL;
#endif
	  strcpy ( lastColumn, cardReader_->columnName (  ) );
	  objective_[column] = 0.0;
	  start[column] = numberElements_;
	  numberColumns_++;
	}
	if ( fabs ( cardReader_->value (  ) ) > smallElement_ ) {
	  if ( numberElements_ == maxElements ) {
	    maxElements = ( 3 * maxElements ) / 2 + 1000;
	    row = reinterpret_cast<COINRowIndex *>
	      (realloc ( row, maxElements * sizeof ( COINRowIndex )));
	    element = reinterpret_cast<double *>
	      (realloc ( element, maxElements * sizeof ( double )));
	  }
	  // get row number
	  COINRowIndex irow = findHash ( cardReader_->rowName (  ) , 0 );

	  if ( irow >= 0 ) {
	    double value = cardReader_->value (  );

	    // check for duplicates
	    if ( irow == numberRows_ ) {
	      // objective
	      if ( objUsed ) {
		numberErrors++;
		if ( numberErrors < 100 ) {
		  handler_->message(COIN_MPS_DUPOBJ,messages_)
		    <<cardReader_->cardNumber()<<cardReader_->card()
		    <<CoinMessageEol;
		} else if (numberErrors > 100000) {
		  handler_->message(COIN_MPS_RETURNING,messages_)
		    <<CoinMessageEol;
		  return numberErrors;
		}
	      } else {
		objUsed = true;
	      }
	      value += objective_[column];
	      if ( fabs ( value ) <= smallElement_ )
		value = 0.0;
	      objective_[column] = value;
	    } else if ( irow < numberRows_ ) {
	      // other free rows will just be discarded so won't get here
	      if ( rowUsed[irow] >= 0 ) {
		element[rowUsed[irow]] += value;
		numberErrors++;
		if ( numberErrors < 100 ) {
		  handler_->message(COIN_MPS_DUPROW,messages_)
		    <<cardReader_->rowName()<<cardReader_->cardNumber()
		    <<cardReader_->card()
		    <<CoinMessageEol;
		} else if (numberErrors > 100000) {
		  handler_->message(COIN_MPS_RETURNING,messages_)
		    <<CoinMessageEol;
		  return numberErrors;
		}
	      } else {
		row[numberElements_] = irow;
		element[numberElements_] = value;
		rowUsed[irow] = numberElements_;
		numberElements_++;
	      }
	    }
	  } else {
	    numberErrors++;
	    if ( numberErrors < 100 ) {
		  handler_->message(COIN_MPS_NOMATCHROW,messages_)
		    <<cardReader_->rowName()<<cardReader_->cardNumber()<<cardReader_->card()
		    <<CoinMessageEol;
	    } else if (numberErrors > 100000) {
	      handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	      return numberErrors;
	    }
	  }
	} else if (cardReader_->value () == STRING_VALUE ) {
	  // tiny element - string
	  const char * s = cardReader_->valueString();
	  assert (*s=='=');
	  // get row number
	  COINRowIndex irow = findHash ( cardReader_->rowName (  ) , 0 );

	  if ( irow >= 0 ) {
	    addString(irow,column,s+1);
	  } else {
	    numberErrors++;
	    if ( numberErrors < 100 ) {
		  handler_->message(COIN_MPS_NOMATCHROW,messages_)
		    <<cardReader_->rowName()<<cardReader_->cardNumber()<<cardReader_->card()
		    <<CoinMessageEol;
	    } else if (numberErrors > 100000) {
	      handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	      return numberErrors;
	    }
	  }
	}
	break;
      case COIN_INTORG:
	inIntegerSet = true;
	break;
      case COIN_INTEND:
	inIntegerSet = false;
	break;
      case COIN_S1_COLUMN:
      case COIN_S2_COLUMN:
      case COIN_S3_COLUMN:
      case COIN_SOSEND:
	std::cout << "** code sos etc later" << std::endl;
	abort (  );
	break;
      default:
	numberErrors++;
	if ( numberErrors < 100 ) {
	  handler_->message(COIN_MPS_BADIMAGE,messages_)<<cardReader_->cardNumber()
						       <<cardReader_->card()
						       <<CoinMessageEol;
	} else if (numberErrors > 100000) {
	  handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	  return numberErrors;
	}
      }
    }
    start[numberColumns_] = numberElements_;
    delete[]rowUsed;
    if ( cardReader_->whichSection (  ) != COIN_RHS_SECTION ) {
      handler_->message(COIN_MPS_BADIMAGE,messages_)<<cardReader_->cardNumber()
						    <<cardReader_->card()
						    <<CoinMessageEol;
      handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
      return numberErrors+100000;
    }
    if (numberColumns_) {
      columnType =
	reinterpret_cast<COINMpsType *> (realloc ( columnType,
						   numberColumns_ * sizeof ( COINMpsType )));
      columnName =
	
	reinterpret_cast<char **> (realloc ( columnName, numberColumns_ * sizeof ( char * )));
      objective_ = reinterpret_cast<double *>
	(realloc ( objective_, numberColumns_ * sizeof ( double )));
    } else {
      columnType =
	reinterpret_cast<COINMpsType *> (realloc ( columnType,
				     sizeof ( COINMpsType )));
      columnName =
	
	reinterpret_cast<char **> (realloc ( columnName, sizeof ( char * )));
      objective_ = reinterpret_cast<double *>
	(realloc ( objective_, sizeof ( double )));
    }
    start = reinterpret_cast<CoinBigIndex *>
      (realloc ( start, ( numberColumns_ + 1 ) * sizeof ( CoinBigIndex )));
    if (numberElements_) {
      row = reinterpret_cast<COINRowIndex *>
	(realloc ( row, numberElements_ * sizeof ( COINRowIndex )));
      element = reinterpret_cast<double *>
	(realloc ( element, numberElements_ * sizeof ( double )));
    } else {
      row = reinterpret_cast<COINRowIndex *>
	(realloc ( row,  sizeof ( COINRowIndex )));
      element = reinterpret_cast<double *>
	(realloc ( element, sizeof ( double )));
    }
    if (numberRows_) {
      rowlower_ = reinterpret_cast<double *> (malloc ( numberRows_ * sizeof ( double )));
      rowupper_ = reinterpret_cast<double *> (malloc ( numberRows_ * sizeof ( double )));
    } else {
      rowlower_ = reinterpret_cast<double *> (malloc ( sizeof ( double )));
      rowupper_ = reinterpret_cast<double *> (malloc ( sizeof ( double )));
    }
    for (i=0;i<numberRows_;i++) {
      rowlower_[i]=-infinity_;
      rowupper_[i]=infinity_;
    }
    objUsed = false;
    memset ( lastColumn, '\0', 200 );
    bool gotRhs = false;

    // need coding for blank rhs
    while ( cardReader_->nextField (  ) == COIN_RHS_SECTION ) {
      COINRowIndex irow;

      switch ( cardReader_->mpsType (  ) ) {
      case COIN_BLANK_COLUMN:
	if ( strcmp ( lastColumn, cardReader_->columnName (  ) ) ) {

	  // skip rest if got a rhs
	  if ( gotRhs ) {
	    while ( cardReader_->nextField (  ) == COIN_RHS_SECTION ) {
	    }
	    break;
	  } else {
	    gotRhs = true;
	    strcpy ( lastColumn, cardReader_->columnName (  ) );
	    // save name of section
	    free(rhsName_);
	    rhsName_=CoinStrdup(cardReader_->columnName());
	  }
	}
	// get row number
	irow = findHash ( cardReader_->rowName (  ) , 0 );
	if ( irow >= 0 ) {
	  double value = cardReader_->value (  );

	  // check for duplicates
	  if ( irow == numberRows_ ) {
	    // objective
	    if ( objUsed ) {
	      numberErrors++;
	      if ( numberErrors < 100 ) {
		  handler_->message(COIN_MPS_DUPOBJ,messages_)
		    <<cardReader_->cardNumber()<<cardReader_->card()
		    <<CoinMessageEol;
	      } else if (numberErrors > 100000) {
		handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
		return numberErrors;
	      }
	    } else {
	      objUsed = true;
	    }
	    if (value==STRING_VALUE) {
	      value=0.0;
	      // tiny element - string
	      const char * s = cardReader_->valueString();
	      assert (*s=='=');
	      addString(irow,numberColumns_,s+1);
	    }
	    objectiveOffset_ += value;
	  } else if ( irow < numberRows_ ) {
	    if ( rowlower_[irow] != -infinity_ ) {
	      numberErrors++;
	      if ( numberErrors < 100 ) {
		handler_->message(COIN_MPS_DUPROW,messages_)
		  <<cardReader_->rowName()<<cardReader_->cardNumber()<<cardReader_->card()
		  <<CoinMessageEol;
	      } else if (numberErrors > 100000) {
		handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
		return numberErrors;
	      }
	    } else {
	      if (value==STRING_VALUE) {
		value=0.0;
		// tiny element - string
		const char * s = cardReader_->valueString();
		assert (*s=='=');
		addString(irow,numberColumns_,s+1);
	      }
	      rowlower_[irow] = value;
	    }
	  }
	} else {
	  numberErrors++;
	  if ( numberErrors < 100 ) {
	    handler_->message(COIN_MPS_NOMATCHROW,messages_)
	      <<cardReader_->rowName()<<cardReader_->cardNumber()<<cardReader_->card()
	      <<CoinMessageEol;
	  } else if (numberErrors > 100000) {
	    handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	    return numberErrors;
	  }
	}
	break;
      default:
	numberErrors++;
	if ( numberErrors < 100 ) {
	  handler_->message(COIN_MPS_BADIMAGE,messages_)<<cardReader_->cardNumber()
						       <<cardReader_->card()
						       <<CoinMessageEol;
	} else if (numberErrors > 100000) {
	  handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	  return numberErrors;
	}
      }
    }
    if ( cardReader_->whichSection (  ) == COIN_RANGES_SECTION ) {
      memset ( lastColumn, '\0', 200 );
      bool gotRange = false;
      COINRowIndex irow;

      // need coding for blank range
      while ( cardReader_->nextField (  ) == COIN_RANGES_SECTION ) {
	switch ( cardReader_->mpsType (  ) ) {
	case COIN_BLANK_COLUMN:
	  if ( strcmp ( lastColumn, cardReader_->columnName (  ) ) ) {

	    // skip rest if got a range
	    if ( gotRange ) {
	      while ( cardReader_->nextField (  ) == COIN_RANGES_SECTION ) {
	      }
	      break;
	    } else {
	      gotRange = true;
	      strcpy ( lastColumn, cardReader_->columnName (  ) );
	      // save name of section
	      free(rangeName_);
	      rangeName_=CoinStrdup(cardReader_->columnName());
	    }
	  }
	  // get row number
	  irow = findHash ( cardReader_->rowName (  ) , 0 );
	  if ( irow >= 0 ) {
	    double value = cardReader_->value (  );

	    // check for duplicates
	    if ( irow == numberRows_ ) {
	      // objective
	      numberErrors++;
	      if ( numberErrors < 100 ) {
		  handler_->message(COIN_MPS_DUPOBJ,messages_)
		    <<cardReader_->cardNumber()<<cardReader_->card()
		    <<CoinMessageEol;
	      } else if (numberErrors > 100000) {
		handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
		return numberErrors;
	      }
	    } else {
	      if ( rowupper_[irow] != infinity_ ) {
		numberErrors++;
		if ( numberErrors < 100 ) {
		  handler_->message(COIN_MPS_DUPROW,messages_)
		    <<cardReader_->rowName()<<cardReader_->cardNumber()<<cardReader_->card()
		    <<CoinMessageEol;
		} else if (numberErrors > 100000) {
		  handler_->message(COIN_MPS_RETURNING,messages_)
		    <<CoinMessageEol;
		  return numberErrors;
		}
	      } else {
		rowupper_[irow] = value;
	      }
	    }
	  } else {
	    numberErrors++;
	    if ( numberErrors < 100 ) {
	      handler_->message(COIN_MPS_NOMATCHROW,messages_)
		<<cardReader_->rowName()<<cardReader_->cardNumber()<<cardReader_->card()
		<<CoinMessageEol;
	    } else if (numberErrors > 100000) {
	      handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	      return numberErrors;
	    }
	  }
	  break;
	default:
	  numberErrors++;
	  if ( numberErrors < 100 ) {
	  handler_->message(COIN_MPS_BADIMAGE,messages_)<<cardReader_->cardNumber()
						       <<cardReader_->card()
						       <<CoinMessageEol;
	  } else if (numberErrors > 100000) {
	    handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	    return numberErrors;
	  }
	}
      }
    }
    stopHash ( 0 );
    // massage ranges
    {
      COINRowIndex irow;

      for ( irow = 0; irow < numberRows_; irow++ ) {
	double lo = rowlower_[irow];
	double up = rowupper_[irow];
	double up2 = rowupper_[irow];	//range

	switch ( rowType[irow] ) {
	case COIN_E_ROW:
	  if ( lo == -infinity_ )
	    lo = 0.0;
	  if ( up == infinity_ ) {
	    up = lo;
	  } else if ( up > 0.0 ) {
	    up += lo;
	  } else {
	    up = lo;
	    lo += up2;
	  }
	  break;
	case COIN_L_ROW:
	  if ( lo == -infinity_ ) {
	    up = 0.0;
	  } else {
	    up = lo;
	    lo = -infinity_;
	  }
	  if ( up2 != infinity_ ) {
	    lo = up - fabs ( up2 );
	  }
	  break;
	case COIN_G_ROW:
	  if ( lo == -infinity_ ) {
	    lo = 0.0;
	    up = infinity_;
	  } else {
	    up = infinity_;
	  }
	  if ( up2 != infinity_ ) {
	    up = lo + fabs ( up2 );
	  }
	  break;
	default:
	  abort();
	}
	rowlower_[irow] = lo;
	rowupper_[irow] = up;
      }
    }
    free ( rowType );
    // default bounds
    if (numberColumns_) {
      collower_ = reinterpret_cast<double *> (malloc ( numberColumns_ * sizeof ( double )));
      colupper_ = reinterpret_cast<double *> (malloc ( numberColumns_ * sizeof ( double )));
    } else {
      collower_ = reinterpret_cast<double *> (malloc ( sizeof ( double )));
      colupper_ = reinterpret_cast<double *> (malloc ( sizeof ( double )));
    }
    for (i=0;i<numberColumns_;i++) {
      collower_[i]=0.0;
      colupper_[i]=infinity_;
    }
    // set up integer region just in case
    if (numberColumns_) 
      integerType_ = reinterpret_cast<char *> (malloc (numberColumns_*sizeof(char)));
    else
      integerType_ = reinterpret_cast<char *> (malloc (sizeof(char)));
    for ( column = 0; column < numberColumns_; column++ ) {
      if ( columnType[column] == COIN_INTORG ) {
	columnType[column] = COIN_UNSET_BOUND;
	integerType_[column] = 1;
      } else {
	integerType_[column] = 0;
      }
    }
    // start hash even if no bound section - to make sure names survive
    startHash ( columnName, numberColumns_ , 1 );
    if ( cardReader_->whichSection (  ) == COIN_BOUNDS_SECTION ) {
      memset ( lastColumn, '\0', 200 );
      bool gotBound = false;

      while ( cardReader_->nextField (  ) == COIN_BOUNDS_SECTION ) {
	if ( strcmp ( lastColumn, cardReader_->columnName (  ) ) ) {

	  // skip rest if got a bound
	  if ( gotBound ) {
	    while ( cardReader_->nextField (  ) == COIN_BOUNDS_SECTION ) {
	    }
	    break;
	  } else {
	    gotBound = true;;
	    strcpy ( lastColumn, cardReader_->columnName (  ) );
	    // save name of section
	    free(boundName_);
	    boundName_=CoinStrdup(cardReader_->columnName());
	  }
	}
	// get column number
	COINColumnIndex icolumn = findHash ( cardReader_->rowName (  ) , 1 );

	if ( icolumn >= 0 ) {
	  double value = cardReader_->value (  );
	  bool ifError = false;

	  switch ( cardReader_->mpsType (  ) ) {
	  case COIN_UP_BOUND:
	    if ( value == -1.0e100 )
	      ifError = true;
	    if (value==STRING_VALUE) {
	      value=1.0e10;
	      // tiny element - string
	      const char * s = cardReader_->valueString();
	      assert (*s=='=');
	      addString(numberRows_+2,icolumn,s+1);
	    }
	    if ( columnType[icolumn] == COIN_UNSET_BOUND ) {
	      if ( value < 0.0 ) {
		collower_[icolumn] = -infinity_;
	      }
	    } else if ( columnType[icolumn] == COIN_LO_BOUND ) {
	      if ( value < collower_[icolumn] ) {
		ifError = true;
	      } else if ( value < collower_[icolumn] + smallElement_ ) {
		value = collower_[icolumn];
	      }
	    } else if ( columnType[icolumn] == COIN_MI_BOUND ) {
	    } else {
	      ifError = true;
	    }
            if (value>1.0e25)
              value=infinity_;
	    colupper_[icolumn] = value;
	    columnType[icolumn] = COIN_UP_BOUND;
	    break;
	  case COIN_LO_BOUND:
	    if ( value == -1.0e100 )
	      ifError = true;
	    if (value==STRING_VALUE) {
	      value=-1.0e10;
	      // tiny element - string
	      const char * s = cardReader_->valueString();
	      assert (*s=='=');
	      addString(numberRows_+1,icolumn,s+1);
	    }
	    if ( columnType[icolumn] == COIN_UNSET_BOUND ) {
	    } else if ( columnType[icolumn] == COIN_UP_BOUND ||
			columnType[icolumn] == COIN_UI_BOUND ) {
	      if ( value > colupper_[icolumn] ) {
		ifError = true;
	      } else if ( value > colupper_[icolumn] - smallElement_ ) {
		value = colupper_[icolumn];
	      }
	    } else {
	      ifError = true;
	    }
            if (value<-1.0e25)
              value=-infinity_;
	    collower_[icolumn] = value;
	    columnType[icolumn] = COIN_LO_BOUND;
	    break;
	  case COIN_FX_BOUND:
	    if ( value == -1.0e100 )
	      ifError = true;
	    if (value==STRING_VALUE) {
	      value=0.0;
	      // tiny element - string
	      const char * s = cardReader_->valueString();
	      assert (*s=='=');
	      addString(numberRows_+1,icolumn,s+1);
	      addString(numberRows_+2,icolumn,s+1);
	    }
	    if ( columnType[icolumn] == COIN_UNSET_BOUND ) {
	    } else if ( columnType[icolumn] == COIN_UI_BOUND ||
			columnType[icolumn] == COIN_BV_BOUND) {
	      // Allow so people can easily put FX's at end
	      double value2 = floor(value);
	      if (fabs(value2-value)>1.0e-12||
		  value2<collower_[icolumn]||
		  value2>colupper_[icolumn]) {
		ifError=true;
	      } else {
		// take off integer list
		assert(integerType_[icolumn] );
		numberIntegers--;
		integerType_[icolumn] = 0;
	      }
	    } else {
	      ifError = true;
	    }
	    collower_[icolumn] = value;
	    colupper_[icolumn] = value;
	    columnType[icolumn] = COIN_FX_BOUND;
	    break;
	  case COIN_FR_BOUND:
	    if ( columnType[icolumn] == COIN_UNSET_BOUND ) {
	    } else {
	      ifError = true;
	    }
	    collower_[icolumn] = -infinity_;
	    colupper_[icolumn] = infinity_;
	    columnType[icolumn] = COIN_FR_BOUND;
	    break;
	  case COIN_MI_BOUND:
	    if ( columnType[icolumn] == COIN_UNSET_BOUND ) {
	      colupper_[icolumn] = COIN_DBL_MAX;
	    } else if ( columnType[icolumn] == COIN_UP_BOUND ) {
	    } else {
	      ifError = true;
	    }
	    collower_[icolumn] = -infinity_;
	    columnType[icolumn] = COIN_MI_BOUND;
	    break;
	  case COIN_PL_BOUND:
	    // change to allow if no upper bound set
	    //if ( columnType[icolumn] == COIN_UNSET_BOUND ) {
	    if (colupper_[icolumn]==infinity_) {
	    } else {
	      ifError = true;
	    }
	    columnType[icolumn] = COIN_PL_BOUND;
	    break;
	  case COIN_UI_BOUND:
	    if (value==STRING_VALUE) {
	      value=1.0e20;
	      // tiny element - string
	      const char * s = cardReader_->valueString();
	      assert (*s=='=');
	      addString(numberRows_+2,icolumn,s+1);
	    }
#if 0
	    if ( value == -1.0e100 ) 
	      ifError = true;
	    if ( columnType[icolumn] == COIN_UNSET_BOUND ) {
	    } else if ( columnType[icolumn] == COIN_LO_BOUND ) {
	      if ( value < collower_[icolumn] ) {
		ifError = true;
	      } else if ( value < collower_[icolumn] + smallElement_ ) {
		value = collower_[icolumn];
	      }
	    } else {
	      ifError = true;
	    }
#else
	    if ( value == -1.0e100 ) {
	       value = infinity_;
	       if (columnType[icolumn] != COIN_UNSET_BOUND &&
		   columnType[icolumn] != COIN_LO_BOUND) {
		  ifError = true;
	       }
	    } else {
	       if ( columnType[icolumn] == COIN_UNSET_BOUND ) {
	       } else if ( columnType[icolumn] == COIN_LO_BOUND ||
                           columnType[icolumn] == COIN_MI_BOUND ) {
		  if ( value < collower_[icolumn] ) {
		     ifError = true;
		  } else if ( value < collower_[icolumn] + smallElement_ ) {
		     value = collower_[icolumn];
		  }
	       } else {
		  ifError = true;
	       }
	    }
#endif
            if (value>1.0e25)
              value=infinity_;
	    colupper_[icolumn] = value;
	    columnType[icolumn] = COIN_UI_BOUND;
	    if ( !integerType_[icolumn] ) {
	      numberIntegers++;
	      integerType_[icolumn] = 1;
	    }
	    break;
	  case COIN_LI_BOUND:
	    if ( value == -1.0e100 )
	      ifError = true;
	    if (value==STRING_VALUE) {
	      value=-1.0e20;
	      // tiny element - string
	      const char * s = cardReader_->valueString();
	      assert (*s=='=');
	      addString(numberRows_+1,icolumn,s+1);
	    }
	    if ( columnType[icolumn] == COIN_UNSET_BOUND ) {
	    } else if ( columnType[icolumn] == COIN_UP_BOUND ||
			columnType[icolumn] == COIN_UI_BOUND ) {
	      if ( value > colupper_[icolumn] ) {
		ifError = true;
	      } else if ( value > colupper_[icolumn] - smallElement_ ) {
		value = colupper_[icolumn];
	      }
	    } else {
	      ifError = true;
	    }
            if (value<-1.0e25)
              value=-infinity_;
	    collower_[icolumn] = value;
	    columnType[icolumn] = COIN_LI_BOUND;
	    if ( !integerType_[icolumn] ) {
	      numberIntegers++;
	      integerType_[icolumn] = 1;
	    }
	    break;
	  case COIN_BV_BOUND:
	    if ( columnType[icolumn] == COIN_UNSET_BOUND ) {
	    } else {
	      ifError = true;
	    }
	    collower_[icolumn] = 0.0;
	    colupper_[icolumn] = 1.0;
	    columnType[icolumn] = COIN_BV_BOUND;
	    if ( !integerType_[icolumn] ) {
	      numberIntegers++;
	      integerType_[icolumn] = 1;
	    }
	    break;
	  default:
	    ifError = true;
	    break;
	  }
	  if ( ifError ) {
	    numberErrors++;
	    if ( numberErrors < 100 ) {
	      handler_->message(COIN_MPS_BADIMAGE,messages_)
		<<cardReader_->cardNumber()
		<<cardReader_->card()
		<<CoinMessageEol;
	    } else if (numberErrors > 100000) {
	      handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	      return numberErrors;
	    }
	  }
	} else {
	  numberErrors++;
	  if ( numberErrors < 100 ) {
	    handler_->message(COIN_MPS_NOMATCHCOL,messages_)
	      <<cardReader_->rowName()<<cardReader_->cardNumber()<<cardReader_->card()
	      <<CoinMessageEol;
	  } else if (numberErrors > 100000) {
	    handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	    return numberErrors;
	  }
	}
      }
    }
    //for (i=0;i<numberSets;i++)
    //delete sets[i];
    numberSets=0;
    //delete [] sets;
    sets=NULL;
    
    // Do SOS if found
    if ( cardReader_->whichSection (  ) == COIN_SOS_SECTION ) {
      // Go to free format
      cardReader_->setFreeFormat(true);
      int numberInSet=0;
      int iType=-1;
      int * which = new int[numberColumns_];
      double * weights = new double[numberColumns_];
      CoinSet ** setsA = new CoinSet * [numberColumns_];
      while ( cardReader_->nextField (  ) == COIN_SOS_SECTION ) {
	if (cardReader_->mpsType()==COIN_S1_BOUND||
	    cardReader_->mpsType()==COIN_S2_BOUND) {
	  if (numberInSet) {
	    CoinSosSet * newSet = new CoinSosSet(numberInSet,which,weights,iType);
	    setsA[numberSets++]=newSet;
	  }
	  numberInSet=0;
	  iType = cardReader_->mpsType()== COIN_S1_BOUND ? 1 : 2;
	  // skip
	  continue;
	}
	// get column number
	COINColumnIndex icolumn = findHash ( cardReader_->columnName (  ) , 1 );
	if ( icolumn >= 0 ) {
	  //integerType_[icolumn]=2;
	  double value = cardReader_->value (  );
	  if (value==-1.0e100)
	    value = atof(cardReader_->rowName()); // try from row name
	  which[numberInSet]=icolumn;
	  weights[numberInSet++]=value;
	} else {
	  numberErrors++;
	  if ( numberErrors < 100 ) {
	    handler_->message(COIN_MPS_NOMATCHCOL,messages_)
	      <<cardReader_->columnName()<<cardReader_->cardNumber()<<cardReader_->card()
	      <<CoinMessageEol;
	  } else if (numberErrors > 100000) {
	    handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	    return numberErrors;
	  }
	}
      }
      if (numberInSet) {
	CoinSosSet * newSet = new CoinSosSet(numberInSet,which,weights,iType);
	setsA[numberSets++]=newSet;
      }
      if (numberSets) {
	sets = new CoinSet * [numberSets];
	memcpy(sets,setsA,numberSets*sizeof(CoinSet **));
      }
      delete [] setsA;
      delete [] which;
      delete [] weights;
    }
    stopHash ( 1 );
    // clean up integers
    if ( !numberIntegers ) {
      free(integerType_);
      integerType_ = NULL;
    } else {
      COINColumnIndex icolumn;

      for ( icolumn = 0; icolumn < numberColumns_; icolumn++ ) {
	if ( integerType_[icolumn] ) {
	  collower_[icolumn] = CoinMax( collower_[icolumn] , -MAX_INTEGER );
	  // if 0 infinity make 0-1 ???
	  if ( columnType[icolumn] == COIN_UNSET_BOUND ) 
	    colupper_[icolumn] = defaultBound_;
	  if ( colupper_[icolumn] > MAX_INTEGER ) 
	    colupper_[icolumn] = MAX_INTEGER;
	  // clean up to allow for bad reads on 1.0e2 etc
	  if (colupper_[icolumn]<1.0e10) {
	    double value = colupper_[icolumn];
	    double value2 = floor(value+0.5);
	    if (value!=value2) {
	      if (fabs(value-value2)<1.0e-5)
		colupper_[icolumn]=value2;
	    }
	  }
	  if (collower_[icolumn]>-1.0e10) {
	    double value = collower_[icolumn];
	    double value2 = floor(value+0.5);
	    if (value!=value2) {
	      if (fabs(value-value2)<1.0e-5)
		collower_[icolumn]=value2;
	    }
	  }
	}
      }
    }
    free ( columnType );
    if ( cardReader_->whichSection (  ) != COIN_ENDATA_SECTION &&
	 cardReader_->whichSection (  ) != COIN_QUAD_SECTION ) {
      handler_->message(COIN_MPS_BADIMAGE,messages_)<<cardReader_->cardNumber()
						    <<cardReader_->card()
						    <<CoinMessageEol;
      handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
      return numberErrors+100000;
    }
  } else {
    // This is very simple format - what should we use?
    COINColumnIndex i;
    
    /* old: 
       FILE * fp = cardReader_->filePointer();
       fscanf ( fp, "%d %d %d\n", &numberRows_, &numberColumns_, &i);
    */
    // new:
    char buffer[1000];
    cardReader_->fileInput ()->gets (buffer, 1000);
    sscanf (buffer, "%d %d %d\n", &numberRows_, &numberColumns_, &i);

    numberElements_  = i; // done this way in case numberElements_ long

    rowlower_ = reinterpret_cast<double *> (malloc ( numberRows_ * sizeof ( double )));
    rowupper_ = reinterpret_cast<double *> (malloc ( numberRows_ * sizeof ( double )));
    for ( i = 0; i < numberRows_; i++ ) {
      int j;

      // old: fscanf ( fp, "%d %lg %lg\n", &j, &rowlower_[i], &rowupper_[i] );
      // new:
      cardReader_->fileInput ()->gets (buffer, 1000);
      sscanf (buffer, "%d %lg %lg\n", &j, &rowlower_[i], &rowupper_[i] );

      assert ( i == j );
    }
    collower_ = reinterpret_cast<double *> (malloc ( numberColumns_ * sizeof ( double )));
    colupper_ = reinterpret_cast<double *> (malloc ( numberColumns_ * sizeof ( double )));
    objective_= reinterpret_cast<double *> (malloc ( numberColumns_ * sizeof ( double )));
    start = reinterpret_cast<CoinBigIndex *> (malloc ((numberColumns_ + 1) *
				       sizeof (CoinBigIndex)));
    row = reinterpret_cast<COINRowIndex *> (malloc (numberElements_ * sizeof (COINRowIndex)));
    element = reinterpret_cast<double *> (malloc (numberElements_ * sizeof (double)));

    start[0] = 0;
    numberElements_ = 0;
    for ( i = 0; i < numberColumns_; i++ ) {
      int j;
      int n;

      /* old:
	 fscanf ( fp, "%d %d %lg %lg %lg\n", &j, &n, 
	          &collower_[i], &colupper_[i],
	          &objective_[i] );
      */
      // new: 
      cardReader_->fileInput ()->gets (buffer, 1000);
      sscanf (buffer, "%d %d %lg %lg %lg\n", &j, &n, 
	      &collower_[i], &colupper_[i], &objective_[i] );

      assert ( i == j );
      for ( j = 0; j < n; j++ ) {
	/* old:
	   fscanf ( fp, "       %d %lg\n", &row[numberElements_],
		 &element[numberElements_] );
	*/
	// new: 
	cardReader_->fileInput ()->gets (buffer, 1000);
	sscanf (buffer, "       %d %lg\n", &row[numberElements_],
		 &element[numberElements_] );

	numberElements_++;
      }
      start[i + 1] = numberElements_;
    }
  }
  // construct packed matrix
  matrixByColumn_ = 
    new CoinPackedMatrix(true,
			numberRows_,numberColumns_,numberElements_,
			element,row,start,NULL);
  free ( row );
  free ( start );
  free ( element );

  handler_->message(COIN_MPS_STATS,messages_)<<problemName_
					    <<numberRows_
					    <<numberColumns_
					    <<numberElements_
					    <<CoinMessageEol;
  return numberErrors;
}
#ifdef COIN_HAS_GLPK
#include "glpk.h"
glp_tran* cbc_glp_tran = NULL;
glp_prob* cbc_glp_prob = NULL;
#endif
/* Read a problem in GMPL (subset of AMPL)  format from the given filenames.
   Thanks to Ted Ralphs - I just looked at his coding rather than look at the GMPL documentation.
 */
int 
CoinMpsIO::readGMPL(const char * modelName, const char * dataName,
                    bool keepNames)
{
#ifdef COIN_HAS_GLPK
  int returnCode;
  gutsOfDestructor();
  // initialize
  cbc_glp_tran = glp_mpl_alloc_wksp();
  // read model
  char name[2000]; // should be long enough
  assert (strlen(modelName)<2000&&(!dataName||strlen(dataName)<2000));
  strcpy(name,modelName);
  returnCode = glp_mpl_read_model(cbc_glp_tran,name,false);
  if (returnCode != 0) {
    // errors
    glp_mpl_free_wksp(cbc_glp_tran);
    cbc_glp_tran = NULL;
    return 1;
  }
  if (dataName) {
    // read data
    strcpy(name,dataName);
    returnCode = glp_mpl_read_data(cbc_glp_tran,name);
    if (returnCode != 0) {
      // errors
      glp_mpl_free_wksp(cbc_glp_tran);
      cbc_glp_tran = NULL;
      return 1;
    }
  }
  // generate model
  returnCode = glp_mpl_generate(cbc_glp_tran,NULL);
  if (returnCode!=0) {
    // errors
    glp_mpl_free_wksp(cbc_glp_tran);
    cbc_glp_tran = NULL;
    return 2;
  }
  cbc_glp_prob = glp_create_prob();
  glp_mpl_build_prob(cbc_glp_tran, cbc_glp_prob);
  // Get number of rows, columns, and elements
  numberRows_=glp_get_num_rows(cbc_glp_prob);
  numberColumns_ = glp_get_num_cols(cbc_glp_prob);
  numberElements_=glp_get_num_nz(cbc_glp_prob);
  int iRow, iColumn;
  CoinBigIndex * start = new CoinBigIndex [numberRows_+1];
  int * index = new int [numberElements_];
  double * element = new double[numberElements_];
  // Row stuff
  rowlower_ = reinterpret_cast<double *> (malloc ( numberRows_ * sizeof ( double )));
  rowupper_ = reinterpret_cast<double *> (malloc ( numberRows_ * sizeof ( double )));
  // and objective
  objective_ = reinterpret_cast<double *> (malloc ( numberColumns_ * sizeof ( double )));
  problemName_= CoinStrdup(glp_get_prob_name(cbc_glp_prob));
  int kRow=0;
  start[0]=0;
  numberElements_=0;
  // spare space for checking
  double * el = new double[numberColumns_];
  int * ind = new int[numberColumns_];
  char ** names = NULL;
  if (keepNames) {
    names = reinterpret_cast<char **> (malloc(numberRows_*sizeof(char *)));
    names_[0] = names;
    numberHash_[0] = numberRows_;
  }
  for (iRow=0; iRow<numberRows_;iRow++) {
    int number = glp_get_mat_row(cbc_glp_prob,iRow+1,ind-1,el-1);
    double rowLower,rowUpper;
    int rowType;
    rowLower = glp_get_row_lb(cbc_glp_prob, iRow+1);
    rowUpper = glp_get_row_ub(cbc_glp_prob, iRow+1);
    rowType  = glp_get_row_type(cbc_glp_prob, iRow+1); 
    switch(rowType) {                                           
    case GLP_LO: 
      rowUpper =  COIN_DBL_MAX;
      break;	 
    case GLP_UP:
      rowLower = -COIN_DBL_MAX;
      break;
    case GLP_FR:
      rowLower = -COIN_DBL_MAX;
       rowUpper =  COIN_DBL_MAX;
     break;
    default:
      break;
    }
    rowlower_[kRow]=rowLower;
    rowupper_[kRow]=rowUpper;
    for (int i=0;i<number;i++) {
      iColumn = ind[i]-1;
      index[numberElements_]=iColumn;
      element[numberElements_++]=el[i];
    }
    if (keepNames) {
      strcpy(name,glp_get_row_name(cbc_glp_prob,iRow+1));
      // could look at name?
      names[kRow]=CoinStrdup(name);
    }
    kRow++;
    start[kRow]=numberElements_;
  }
  delete [] el;
  delete [] ind;

   // FIXME why this variable is not used?
  bool minimize=(glp_get_obj_dir(cbc_glp_prob)==GLP_MAX ? false : true);
   // sign correct?
  objectiveOffset_ = glp_get_obj_coef(cbc_glp_prob, 0);
  for (int i=0;i<numberColumns_;i++)
    objective_[i]=glp_get_obj_coef(cbc_glp_prob, i+1);
  if (!minimize) {
  for (int i=0;i<numberColumns_;i++)
    objective_[i]=-objective_[i];
    handler_->message(COIN_GENERAL_INFO,messages_)<<
      " CoinMpsIO::readGMPL(): Maximization problem reformulated as minimization"
						  <<CoinMessageEol;
    objectiveOffset_ = -objectiveOffset_;
  }

  // Matrix
  matrixByColumn_ = new CoinPackedMatrix(false,numberColumns_,numberRows_,numberElements_,
                                  element,index,start,NULL);
  matrixByColumn_->reverseOrdering();
  delete [] element;
  delete [] start;
  delete [] index;
  // Now do columns
  collower_ = reinterpret_cast<double *> (malloc ( numberColumns_ * sizeof ( double )));
  colupper_ = reinterpret_cast<double *> (malloc ( numberColumns_ * sizeof ( double )));
  integerType_ = reinterpret_cast<char *> (malloc (numberColumns_*sizeof(char)));
  if (keepNames) {
    names = reinterpret_cast<char **> (malloc(numberColumns_*sizeof(char *)));
    names_[1] = names;
    numberHash_[1] = numberColumns_;
  }
  int numberIntegers=0;
  for (iColumn=0; iColumn<numberColumns_;iColumn++) {
    double columnLower = glp_get_col_lb(cbc_glp_prob, iColumn+1);
    double columnUpper = glp_get_col_ub(cbc_glp_prob, iColumn+1);
    int columnType = glp_get_col_type(cbc_glp_prob, iColumn+1);
    switch(columnType) {                                           
    case GLP_LO: 
      columnUpper =  COIN_DBL_MAX;
      break;	 
    case GLP_UP:
      columnLower = -COIN_DBL_MAX;
      break;
    case GLP_FR:
      columnLower = -COIN_DBL_MAX;
      columnUpper =  COIN_DBL_MAX;
      break;
    default:
      break;
    }
    collower_[iColumn]=columnLower;
    colupper_[iColumn]=columnUpper;
    columnType = glp_get_col_kind(cbc_glp_prob,iColumn+1);
    if (columnType==GLP_IV) {
      integerType_[iColumn]=1;
      numberIntegers++;
      //assert ( collower_[iColumn] >= -MAX_INTEGER );
      if ( collower_[iColumn] < -MAX_INTEGER ) 
        collower_[iColumn] = -MAX_INTEGER;
      if ( colupper_[iColumn] > MAX_INTEGER ) 
        colupper_[iColumn] = MAX_INTEGER;
    } else if (columnType==GLP_BV) {
      numberIntegers++;
      integerType_[iColumn]=1;
      collower_[iColumn]=0.0;
      colupper_[iColumn]=1.0;
    } else {
      integerType_[iColumn]=0;
    }
    if (keepNames) {
      strcpy(name,glp_get_col_name(cbc_glp_prob,iColumn+1));
      // could look at name?
      names[iColumn]=CoinStrdup(name);
    }
  }
  // leave in case report needed
  //glp_free(cbc_glp_prob);
  //glp_mpl_free_wksp(cbc_glp_tran);
  //glp_free_env();
  if ( !numberIntegers ) {
    free(integerType_);
    integerType_ = NULL;
  }
  if(handler_)
    handler_->message(COIN_MPS_STATS,messages_)<<problemName_
					    <<numberRows_
					    <<numberColumns_
					    <<numberElements_
					    <<CoinMessageEol;
  return 0;
#else
  printf("GLPK is not available\n");
  abort();
  return 1;
#endif
}
//------------------------------------------------------------------
// Read gams files
//------------------------------------------------------------------
int CoinMpsIO::readGms(const char * filename,  const char * extension,bool convertObjective)
{
  convertObjective_=convertObjective;
  // Deal with filename - +1 if new, 0 if same as before, -1 if error
  CoinFileInput *input = 0;
  int returnCode = dealWithFileName(filename,extension,input);
  if (returnCode<0) {
    return -1;
  } else if (returnCode>0) {
    delete cardReader_;
    cardReader_ = new CoinMpsCardReader ( input, this);
  }
  int numberSets=0;
  CoinSet ** sets=NULL;
  returnCode = readGms(numberSets,sets);
  for (int i=0;i<numberSets;i++)
    delete sets[i];
  delete [] sets;
  return returnCode;
}
int CoinMpsIO::readGms(const char * filename,  const char * extension,
		       int & numberSets,CoinSet ** &sets)
{
  // Deal with filename - +1 if new, 0 if same as before, -1 if error
  CoinFileInput *input = 0;
  int returnCode = dealWithFileName(filename,extension,input);
  if (returnCode<0) {
    return -1;
  } else if (returnCode>0) {
    delete cardReader_;
    cardReader_ = new CoinMpsCardReader ( input, this);
  }
  return readGms(numberSets,sets);
}
int CoinMpsIO::readGms(int & /*numberSets*/,CoinSet ** &/*sets*/)
{
  // First version expects comments giving size
  numberRows_ = 0;
  numberColumns_ = 0;
  numberElements_ = 0;
  bool gotName=false;
  bool minimize=false;
  char objName[COIN_MAX_FIELD_LENGTH];
  int decodeType=-1;
  while(!gotName) {
    if (cardReader_->nextGmsField(0)<0) {
      handler_->message(COIN_MPS_EOF,messages_)<<fileName_
                                               <<CoinMessageEol;
      return -3;
    } else {
      char * card = cardReader_->mutableCard();
      if (card[0]!='*') {
        // finished preamble without finding name
        printf("bad gms file\n");
        return -1;
      } else {
        // skip * and find next
        char * next = nextNonBlank(card+1);
        if (!next)
          continue;
        if (decodeType>=0) {
          // in middle of getting a total
          if (!strncmp(next,"Total",5)) {
            // next line wanted
            decodeType+=100;
          } else if (decodeType>=100) {
            decodeType -= 100;
            int number = atoi(next);
            assert (number>0);
            if (decodeType==0)
              numberRows_=number;
            else if (decodeType==1)
              numberColumns_=number;
            else
              numberElements_=number;
            decodeType=-1;
          }
        } else if (!strncmp(next,"Equation",8)) {
          decodeType=0;
        } else if (!strncmp(next,"Variable",8)) {
          decodeType=1;
        } else if (!strncmp(next,"Nonzero",7)) {
          decodeType=2;
        } else if (!strncmp(next,"Solve",5)) {
          decodeType=-1;
          gotName=true;
          assert (numberRows_>0&&numberColumns_>0&&numberElements_>0);
          next = cardReader_->nextBlankOr(next+5);
          char name[100];
          char * put=name;
          next= nextNonBlank(next);
          while(*next!=' '&&*next!='\t') {
            *put = *next;
            put++;
            next++;
          }
          *put='\0';
          assert (put-name<100);
          free(problemName_);
          problemName_=CoinStrdup(name);
          next = strchr(next,';');
          assert (next);
          // backup
          while(*next!=' '&&*next!='\t') {
            next--;
          }
          cardReader_->setPosition(next);
#ifdef NDEBUG
          cardReader_->nextGmsField(1);
#else
          int returnCode = cardReader_->nextGmsField(1);
          assert (!returnCode);
#endif
          next = strchr(next,';');
          cardReader_->setPosition(next+1);
          strcpy(objName,cardReader_->columnName());
          char * semi = strchr(objName,';');
          if (semi)
            *semi='\0';
	  if (strstr(card,"minim")) {
	    minimize=true;
	  } else {
	    assert (strstr(card,"maxim"));
	    minimize=false;
	  }
        } else {
          decodeType=-1;
        }
      }
    }
  }

  objectiveOffset_ = 0.0;
  rowlower_ = reinterpret_cast<double *> (malloc ( numberRows_ * sizeof ( double )));
  rowupper_ = reinterpret_cast<double *> (malloc ( numberRows_ * sizeof ( double )));
  collower_ = reinterpret_cast<double *> (malloc ( numberColumns_ * sizeof ( double )));
  colupper_ = reinterpret_cast<double *> (malloc ( numberColumns_ * sizeof ( double )));
  objective_= reinterpret_cast<double *> (malloc ( numberColumns_ * sizeof ( double )));
  CoinBigIndex *start = reinterpret_cast<CoinBigIndex *> (malloc ((numberRows_ + 1) *
						   sizeof (CoinBigIndex)));
  COINColumnIndex * column = reinterpret_cast<COINRowIndex *> (malloc (numberElements_ * sizeof (COINRowIndex)));
  double *element = reinterpret_cast<double *> (malloc (numberElements_ * sizeof (double)));
  COINMpsType *rowType =
    reinterpret_cast<COINMpsType *> (malloc ( numberRows_ * sizeof ( COINMpsType )));
  char **rowName = reinterpret_cast<char **> (malloc ( numberRows_ * sizeof ( char * )));
  COINMpsType *columnType = reinterpret_cast<COINMpsType *>
    (malloc ( numberColumns_ * sizeof ( COINMpsType )));
  char **columnName = reinterpret_cast<char **> (malloc ( numberColumns_ * sizeof ( char * )));

  start[0] = 0;
  numberElements_ = 0;

  int numberErrors = 0;
  int i;
  COINColumnIndex numberIntegers = 0;

  // expect Variables
  int returnCode;
  returnCode = cardReader_->nextGmsField(1);
  assert (!returnCode&&!strcmp(cardReader_->columnName(),"Variables"));
  for (i=0;i<numberColumns_;i++) {
    returnCode = cardReader_->nextGmsField(1);
    assert (!returnCode);
    char * next = cardReader_->getPosition();
    if (*next=='\0') {
      // eol - expect , at beginning of next line
      returnCode = cardReader_->nextGmsField(0);
      assert (!returnCode);
      next = strchr(cardReader_->mutableCard(),',');
      assert (next);
    }
    assert (*next==','||*next==';');
    cardReader_->setPosition(next+1);
    columnName[i]=CoinStrdup(cardReader_->columnName());
    // Default is free? 
    collower_[i]=-COIN_DBL_MAX;
    // Surely not - check
    collower_[i]=0.0;
    colupper_[i]=COIN_DBL_MAX;
    objective_[i]=0.0;
    columnType[i]=COIN_UNSET_BOUND;
  }
  startHash ( columnName, numberColumns_ , 1 );
  integerType_ = reinterpret_cast<char *> (malloc (numberColumns_*sizeof(char)));
  memset(integerType_,0,numberColumns_);
  // Lists come in various flavors - I don't know many now
  // 0 - Positive
  // 1 - Binary
  // -1 end
  int listType=10;
  while (listType>=0) {
    returnCode=cardReader_->nextGmsField(1);
    assert (!returnCode);
    listType=-1;
    if (!strcmp(cardReader_->columnName(),"Positive")) {
      listType=0;
    } else if (!strcmp(cardReader_->columnName(),"Binary")) {
      listType=1;
    } else if (!strcmp(cardReader_->columnName(),"Integer")) {
      listType=2;
    } else {
      break;
    }
    // skip Variables
    returnCode=cardReader_->nextGmsField(1);
    assert (!returnCode);
    assert (!strcmp(cardReader_->columnName(),"Variables"));

    // Go through lists
    bool inList=true;
    while (inList) {
      returnCode=cardReader_->nextGmsField(1);
      assert (!returnCode);
      char * next = cardReader_->getPosition();
      if (*next=='\0') {
        // eol - expect , at beginning of next line
        returnCode = cardReader_->nextGmsField(0);
        assert (!returnCode);
        next = strchr(cardReader_->mutableCard(),',');
        assert (next);
      }
      assert (*next==','||*next==';');
      cardReader_->setPosition(next+1);
      inList=(*next==',');
      int iColumn = findHash(cardReader_->columnName(),1);
      assert (iColumn>=0);
      if (listType==0) {
        collower_[iColumn]=0.0;
      } else if (listType==1) {
        collower_[iColumn]=0.0;
        colupper_[iColumn]=1.0;
        columnType[iColumn]=COIN_BV_BOUND;
	integerType_[iColumn] = 1;
        numberIntegers++;
      } else if (listType==2) {
        collower_[iColumn]=0.0;
        columnType[iColumn]=COIN_UI_BOUND;
	integerType_[iColumn] = 1;
        numberIntegers++;
      }
    }
  }
  // should be equations
  assert (!strcmp(cardReader_->columnName(),"Equations"));
  for (i=0;i<numberRows_;i++) {
    returnCode = cardReader_->nextGmsField(1);
    assert (!returnCode);
    char * next = cardReader_->getPosition();
    if (*next=='\0') {
      // eol - expect , at beginning of next line
      returnCode = cardReader_->nextGmsField(0);
      assert (!returnCode);
      next = strchr(cardReader_->mutableCard(),',');
      assert (next);
    }
    assert (*next==','||*next==';');
    cardReader_->setPosition(next+1);
    rowName[i]=CoinStrdup(cardReader_->columnName());
    // Default is free?
    rowlower_[i]=-COIN_DBL_MAX;
    rowupper_[i]=COIN_DBL_MAX;
    rowType[i]=COIN_N_ROW;
  }
  startHash ( rowName, numberRows_ , 0 );
  const double largeElement = 1.0e14;
  int numberTiny=0;
  int numberLarge=0;
  // For now expect just equations so do loop
  for (i=0;i<numberRows_;i++) {
    returnCode = cardReader_->nextGmsField(1);
    assert (!returnCode);
    char * next = cardReader_->getPosition();
    assert (*next==' ');
    char rowName[COIN_MAX_FIELD_LENGTH];
    strcpy(rowName,cardReader_->columnName());
    char * dot = strchr(rowName,'.');
    assert (dot); 
    *dot='\0';
    assert (*(dot+1)=='.');
#ifndef NDEBUG
    int iRow = findHash(rowName,0);
    assert (i==iRow);
#endif
    returnCode=0;
    while(!returnCode) {
      returnCode = cardReader_->nextGmsField(3);
      assert (returnCode==0||returnCode==2);
      if (returnCode==2)
        break;
      int iColumn = findHash(cardReader_->columnName(),1);
      if (iColumn>=0) {
	column[numberElements_]=iColumn;
	double value = cardReader_->value();
	if (fabs(value)<smallElement_)
	  numberTiny++;
	else if (fabs(value)>largeElement)
	  numberLarge++;
	element[numberElements_++]=value;
      } else {
	// may be string
	char temp[100];
	strcpy(temp,cardReader_->columnName());
	char * ast = strchr(temp,'*');
	if (!ast) {
	  assert (iColumn>=0);
	} else {
	  assert (allowStringElements_);
	  *ast='\0';
	  if (allowStringElements_==1)
	    iColumn = findHash(temp,1);
	  else
	    iColumn = findHash(ast+1,1);
	  assert (iColumn>=0);
	  char temp2[100];
	  temp2[0]='\0';
	  double value = cardReader_->value();
	  if (value&&value!=1.0) 
	    sprintf(temp2,"%g*",value);
	  if (allowStringElements_==1)
	    strcat(temp2,ast+1);
	  else
	    strcat(temp2,temp);
	  addString(i,iColumn,temp2);
	}
      }
    }
    start[i+1]=numberElements_;
    next=cardReader_->getPosition();
    // what about ranges?
    COINMpsType type=COIN_N_ROW;
    if (!strncmp(next,"=E=",3))
      type=COIN_E_ROW;
    else if (!strncmp(next,"=G=",3))
      type=COIN_G_ROW;
    else if (!strncmp(next,"=L=",3))
      type=COIN_L_ROW;
    assert (type!=COIN_N_ROW);
    cardReader_->setPosition(next+3);
    returnCode = cardReader_->nextGmsField(2);
    assert (!returnCode);
    if (type==COIN_E_ROW) {
      rowlower_[i]=cardReader_->value();
      rowupper_[i]=cardReader_->value();
    } else if (type==COIN_G_ROW) {
      rowlower_[i]=cardReader_->value();
    } else if (type==COIN_L_ROW) {
      rowupper_[i]=cardReader_->value();
    }
    rowType[i]=type;
    // and skip ;
#ifdef NDEBUG
    cardReader_->nextGmsField(5);
#else
    returnCode = cardReader_->nextGmsField(5);
    assert (!returnCode);
#endif
  }
  // Now non default bounds
  while (true) {
    returnCode=cardReader_->nextGmsField(0);
    if (returnCode<0)
      break;
    // if there is a . see if valid name
    char * card = cardReader_->mutableCard();
    char * dot = strchr(card,'.');
    if (dot) {
      *dot='\0';
      int iColumn = findHash(card,1);
      if (iColumn>=0) {
        // bound
        char * next = strchr(dot+1,'=');
        assert (next);
        double value =atof(next+1);
        if (!strncmp(dot+1,"fx",2)) {
          collower_[iColumn]=value;
          colupper_[iColumn]=value;
        } else if (!strncmp(dot+1,"up",2)) {
          colupper_[iColumn]=value;
        } else if (!strncmp(dot+1,"lo",2)) {
          collower_[iColumn]=value;
        }
      }
      // may be two per card
      char * semi = strchr(dot+1,';');
      dot = NULL;
      if (semi)
	dot = strchr(semi+1,'.');
      if (dot) {
	char * next= nextNonBlank(semi+1);
	dot = strchr(next,'.');
	assert (dot);
	*dot='\0';
	assert (iColumn==findHash(next,1));
        // bound
        next = strchr(dot+1,'=');
        assert (next);
        double value =atof(next+1);
        if (!strncmp(dot+1,"fx",2)) {
          collower_[iColumn]=value;
	  abort();
          colupper_[iColumn]=value;
        } else if (!strncmp(dot+1,"up",2)) {
          colupper_[iColumn]=value;
        } else if (!strncmp(dot+1,"lo",2)) {
          collower_[iColumn]=value;
        }
	// may be two per card
	semi = strchr(dot+1,';');
	assert (semi);
      }
    }
  }
  // Objective
  int iObjCol = findHash(objName,1);
  int iObjRow=-1;
  assert (iObjCol>=0);
  if (!convertObjective_) {
    objective_[iObjCol]=minimize ? 1.0 : -1.0;
  } else {
    // move column stuff
    COINColumnIndex iColumn;
    free(names_[1][iObjCol]);
    for ( iColumn = iObjCol+1; iColumn < numberColumns_; iColumn++ ) {
      integerType_[iColumn-1]=integerType_[iColumn];
      collower_[iColumn-1]=collower_[iColumn];
      colupper_[iColumn-1]=colupper_[iColumn];
      names_[1][iColumn-1]=names_[1][iColumn];
    }
    numberHash_[1]--;
    numberColumns_--;
    double multiplier = minimize ? 1.0 : -1.0;
    // but swap
    multiplier *= -1.0;
    int iRow;
    CoinBigIndex nel=0;
    CoinBigIndex last=0;
    int kRow=0;
    for (iRow=0;iRow<numberRows_;iRow++) {
      CoinBigIndex j;
      bool found=false;
      for (j=last;j<start[iRow+1];j++) {
	int iColumn = column[j];
	if (iColumn!=iObjCol) {
	  column[nel]=(iColumn<iObjCol) ? iColumn : iColumn-1;
	  element[nel++]=element[j];
	} else {
	  found=true;
	  assert (element[j]==1.0);
	  break;
	}
      }
      if (!found) {
	last=start[iRow+1];
	rowlower_[kRow]=rowlower_[iRow];
	rowupper_[kRow]=rowupper_[iRow];
	names_[0][kRow]=names_[0][iRow];
	start[kRow+1]=nel;
	kRow++;
      } else {
	free(names_[0][iRow]);
	iObjRow = iRow;
	for (j=last;j<start[iRow+1];j++) {
	  int iColumn = column[j];
	  if (iColumn!=iObjCol) {
	    if (iColumn>iObjCol)
	      iColumn --;
	    objective_[iColumn]=multiplier * element[j];
	  }
	}
	nel=start[kRow];
	last=start[iRow+1];
      }
    }
    numberRows_=kRow;
    assert (iObjRow>=0);
    numberHash_[0]--;
  }
  stopHash(0);
  stopHash(1);
  // clean up integers
  if ( !numberIntegers ) {
    free(integerType_);
    integerType_ = NULL;
  } else {
    COINColumnIndex iColumn;
    for ( iColumn = 0; iColumn < numberColumns_; iColumn++ ) {
      if ( integerType_[iColumn] ) {
        //assert ( collower_[iColumn] >= -MAX_INTEGER );
        if ( collower_[iColumn] < -MAX_INTEGER ) 
          collower_[iColumn] = -MAX_INTEGER;
        if ( colupper_[iColumn] > MAX_INTEGER ) 
          colupper_[iColumn] = MAX_INTEGER;
      }
    }
  }
  free ( columnType );
  free ( rowType );
  if (numberStringElements()&&convertObjective_) {
    int numberElements = numberStringElements();
    for (int i=0;i<numberElements;i++) {
      char * line = stringElements_[i];
      int iRow;
      int iColumn;
      sscanf(line,"%d,%d,",&iRow,&iColumn);
      bool modify=false;
      if (iRow>iObjRow) {
	modify=true;
	iRow--;
      }
      if (iColumn>iObjCol) {
	modify=true;
	iColumn--;
      }
      if (modify) {
	char temp[500];
	const char * pos = strchr(line,',');
	assert (pos);
	pos = strchr(pos+1,',');
	assert (pos);
	pos++;
	sprintf(temp,"%d,%d,%s",iRow,iColumn,pos);
	free(line);
	stringElements_[i]=CoinStrdup(temp);
      }
    }
  }
// construct packed matrix and convert to column format
  CoinPackedMatrix matrixByRow(false,
                               numberColumns_,numberRows_,numberElements_,
                               element,column,start,NULL);
  free ( column );
  free ( start );
  free ( element );
  matrixByColumn_= new CoinPackedMatrix();
  matrixByColumn_->setExtraGap(0.0);
  matrixByColumn_->setExtraMajor(0.0);
  matrixByColumn_->reverseOrderedCopyOf(matrixByRow);
  if (!convertObjective_)
    assert (matrixByColumn_->getVectorLengths()[iObjCol]==1);
  
  handler_->message(COIN_MPS_STATS,messages_)<<problemName_
					    <<numberRows_
					    <<numberColumns_
					    <<numberElements_
					    <<CoinMessageEol;
  if ((numberTiny||numberLarge)&&handler_->logLevel()>3)
    printf("There were %d coefficients < %g and %d > %g\n",
           numberTiny,smallElement_,numberLarge,largeElement);
  return numberErrors;
}
/* Read a basis in MPS format from the given filename.
   If VALUES on NAME card and solution not NULL fills in solution
   status values as for CoinWarmStartBasis (but one per char)
   
   Use "stdin" or "-" to read from stdin.
*/
int 
CoinMpsIO::readBasis(const char *filename, const char *extension ,
		     double * solution, unsigned char * rowStatus, unsigned char * columnStatus,
		     const std::vector<std::string> & colnames,int numberColumns,
		     const std::vector<std::string> & rownames, int numberRows)
{
  // Deal with filename - +1 if new, 0 if same as before, -1 if error
  CoinFileInput *input = 0;
  int returnCode = dealWithFileName(filename,extension,input);
  if (returnCode<0) {
    return -1;
  } else if (returnCode>0) {
    delete cardReader_;
    cardReader_ = new CoinMpsCardReader ( input, this);
  }

  cardReader_->readToNextSection();

  if ( cardReader_->whichSection (  ) == COIN_NAME_SECTION ) {
    // Get whether to use values (passed back by freeFormat)
    if (!cardReader_->freeFormat())
      solution = NULL;
    
  } else if ( cardReader_->whichSection (  ) == COIN_UNKNOWN_SECTION ) {
    handler_->message(COIN_MPS_BADFILE1,messages_)<<cardReader_->card()
						  <<1
						 <<fileName_
						 <<CoinMessageEol;
    if (cardReader_->fileInput()->getReadType()!="plain") 
      handler_->message(COIN_MPS_BADFILE2,messages_)
        <<cardReader_->fileInput()->getReadType()
        <<CoinMessageEol;

    return -2;
  } else if ( cardReader_->whichSection (  ) != COIN_EOF_SECTION ) {
    return -4;
  } else {
    handler_->message(COIN_MPS_EOF,messages_)<<fileName_
					    <<CoinMessageEol;
    return -3;
  }
  numberRows_=numberRows;
  numberColumns_=numberColumns;
  // bas file - always read in free format
  bool gotNames;
  if (rownames.size()!=static_cast<unsigned int> (numberRows_)||
      colnames.size()!=static_cast<unsigned int> (numberColumns_)) {
    gotNames = false;
  } else {
    gotNames=true;
    numberHash_[0]=numberRows_;
    numberHash_[1]=numberColumns_;
    names_[0] = reinterpret_cast<char **> (malloc(numberRows_ * sizeof(char *)));
    names_[1] = reinterpret_cast<char **> (malloc (numberColumns_ * sizeof(char *)));
    const char** rowNames = const_cast<const char **>(names_[0]);
    const char** columnNames = const_cast<const char **>(names_[1]);
    int i;
    for (i = 0; i < numberRows_; ++i) {
      rowNames[i] = rownames[i].c_str();
    }
    for (i = 0; i < numberColumns_; ++i) {
      columnNames[i] = colnames[i].c_str();
    }
    startHash ( const_cast<char **>(rowNames), numberRows , 0 );
    startHash ( const_cast<char **>(columnNames), numberColumns , 1 );
  }
  cardReader_->setWhichSection(COIN_BASIS_SECTION);
  cardReader_->setFreeFormat(true);
  // below matches CoinWarmStartBasis,
  const unsigned char basic = 0x01;
  const unsigned char atLowerBound = 0x03;
  const unsigned char atUpperBound = 0x02;
  while ( cardReader_->nextField (  ) == COIN_BASIS_SECTION ) {
    // Get type and column number
    int iColumn;
    if (gotNames) {
      iColumn = findHash (cardReader_->columnName(),1);
    } else {
      // few checks 
      char check;
      sscanf(cardReader_->columnName(),"%c%d",&check,&iColumn);
      assert (check=='C'&&iColumn>=0);
      if (iColumn>=numberColumns_)
	iColumn=-1;
    }
    if (iColumn>=0) {
      double value = cardReader_->value (  );
      if (solution && value>-1.0e50)
	solution[iColumn]=value;
      int iRow=-1;
      switch ( cardReader_->mpsType (  ) ) {
      case COIN_BS_BASIS:
	columnStatus[iColumn]=  basic;
	break;
      case COIN_XL_BASIS:
	columnStatus[iColumn]= basic;
	// get row number
	if (gotNames) {
	  iRow = findHash (cardReader_->rowName(),0);
	} else {
	  // few checks 
	  char check;
	  sscanf(cardReader_->rowName(),"%c%d",&check,&iRow);
	  assert (check=='R'&&iRow>=0);
	  if (iRow>=numberRows_)
	    iRow=-1;
	}
	if ( iRow >= 0 ) {
	  rowStatus[iRow] = atLowerBound;
	}
	break;
      case COIN_XU_BASIS:
	columnStatus[iColumn]= basic;
	// get row number
	if (gotNames) {
	  iRow = findHash (cardReader_->rowName(),0);
	} else {
	  // few checks 
	  char check;
	  sscanf(cardReader_->rowName(),"%c%d",&check,&iRow);
	  assert (check=='R'&&iRow>=0);
	  if (iRow>=numberRows_)
	    iRow=-1;
	}
	if ( iRow >= 0 ) {
	  rowStatus[iRow] = atUpperBound;
	}
	break;
      case COIN_LL_BASIS:
	columnStatus[iColumn]= atLowerBound;
	break;
      case COIN_UL_BASIS:
	columnStatus[iColumn]= atUpperBound;
	break;
      default:
	break;
      }
    }
  }
  if (gotNames) {
    stopHash ( 0 );
    stopHash ( 1 );
    free(names_[0]);
    names_[0]=NULL;
    numberHash_[0]=0;
    free(names_[1]);
    names_[1]=NULL;
    numberHash_[1]=0;
    delete[] hash_[0];
    delete[] hash_[1];
    hash_[0]=0;
    hash_[1]=0;
  }
  if ( cardReader_->whichSection (  ) != COIN_ENDATA_SECTION) {
    handler_->message(COIN_MPS_BADIMAGE,messages_)<<cardReader_->cardNumber()
						  <<cardReader_->card()
						  <<CoinMessageEol;
    handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
    return -1;
  } else {
    return solution ? 1 : 0;
  }
}

//------------------------------------------------------------------

// Function to create row name field
static void
convertRowName(int formatType, const char * name, char outputRow[100])
{
  strcpy(outputRow,name);
  if (!formatType) {
    int i;
    // pad out to 8
    for (i=0;i<8;i++) {
      if (outputRow[i]=='\0')
	break;
    }
    for (;i<8;i++) 
      outputRow[i]=' ';
    outputRow[8]='\0';
  } else if (formatType>1&&formatType<8) {
    int i;
    // pad out to 8
    for (i=0;i<8;i++) {
      if (outputRow[i]=='\0')
	break;
    }
    for (;i<8;i++) 
      outputRow[i]=' ';
    outputRow[8]='\0';
  }
}
// Function to return number in most efficient way
// Also creates row name field
/* formatType is
   0 - normal and 8 character names
   1 - extra accuracy
   2 - IEEE hex - INTEL
   3 - IEEE hex - not INTEL
*/
static void
convertDouble(int section,int formatType, double value, char outputValue[24],
	      const char * name, char outputRow[100])
{
  convertRowName(formatType,name,outputRow);
  CoinConvertDouble(section,formatType&3,value,outputValue);
}
// Function to return number in most efficient way
/* formatType is
   0 - normal and 8 character names
   1 - extra accuracy
   2 - IEEE hex - INTEL
   3 - IEEE hex - not INTEL
*/
void
CoinConvertDouble(int section, int formatType, double value, char outputValue[24])
{
  if (formatType==0) {
    bool stripZeros=true;
    if (fabs(value)<1.0e40) {
      int power10, decimal;
      if (value>=0.0) {
	power10 =static_cast<int> (log10(value));
	if (power10<9&&power10>-4) {
	  decimal = CoinMin(10,10-power10);
	  char format[8];
	  sprintf(format,"%%12.%df",decimal);
	  sprintf(outputValue,format,value);
	} else {
	  sprintf(outputValue,"%13.7g",value);
	  stripZeros=false;
	}
      } else {
	power10 =static_cast<int> (log10(-value))+1;
	if (power10<8&&power10>-3) {
	  decimal = CoinMin(9,9-power10);
	  char format[8];
	  sprintf(format,"%%12.%df",decimal);
	  sprintf(outputValue,format,value);
	} else {
	  sprintf(outputValue,"%13.6g",value);
	  stripZeros=false;
	}
      }
      if (stripZeros) {
	// take off trailing 0
	int j;
	for (j=11;j>=0;j--) {
	  if (outputValue[j]=='0')
	    outputValue[j]=' ';
	  else
	    break;
	}
      } else {
	// still need to make sure fits in 12 characters
	char * e = strchr(outputValue,'e');
	if (!e) {
	  // no e but better make sure fits in 12
          if (outputValue[12]!=' '&&outputValue[12]!='\0') {
            assert (outputValue[0]==' ');
            int j;
            for (j=0;j<12;j++) 
              outputValue[j]=outputValue[j+1];
          }
	  outputValue[12]='\0';
	} else {
	  // e take out 0s
	  int j = static_cast<int>((e-outputValue))+1;
	  int put = j+1;
	  assert(outputValue[j]=='-'||outputValue[j]=='+');
	  for ( j = put ; j < 14 ; j++) {
	    if (outputValue[j]!='0')
	      break;
	  }
	  if (j == put) {
	    // we need to lose something
	    // try taking out blanks
	    if (outputValue[0]==' ') {
	      // skip blank
	      j=1;
	      put=0;
	    } else {
	      // rounding will be wrong but ....
	      put -= 3; // points to one before e
	      j -= 2; // points to e
	    }
	  }
	  // copy rest
	  for (  ; j < 14 ; j++) {
	    outputValue[put++] = outputValue[j];
	  }
	}
      }
      // overwrite if very very small
      if (fabs(value)<1.0e-20) 
	strcpy(outputValue,"0.0");
    } else {
      if (section==2) {
        outputValue[0]= '\0'; // needs no value
      } else {
        // probably error ... but ....
        sprintf(outputValue,"%12.6g",value);
      }
    }
    int i;
    // pad out to 12
    for (i=0;i<12;i++) {
      if (outputValue[i]=='\0')
	break;
    }
    for (;i<12;i++) 
      outputValue[i]=' ';
    outputValue[12]='\0';
  } else if (formatType==1) {
    if (fabs(value)<1.0e40) {
      memset(outputValue,' ',24);
      sprintf(outputValue,"%.16g",value);
      // take out blanks
      int i=0;
      int j;
      for (j=0;j<23;j++) {
	if (outputValue[j]!=' ')
	  outputValue[i++]=outputValue[j];
      }
      outputValue[i]='\0';
    } else {
      if (section==2) {
        outputValue[0]= '\0'; // needs no value
      } else {
        // probably error ... but ....
        sprintf(outputValue,"%12.6g",value);
      }
    }
  } else {
    // IEEE
    // ieee - 3 bytes go to 2
    assert (sizeof(double)==8*sizeof(char));
    assert (sizeof(unsigned short) == 2*sizeof(char));
    unsigned short shortValue[4];
    memcpy(shortValue,&value,sizeof(double));
    outputValue[12]='\0';
    if (formatType==2) {
      // INTEL
      char * thisChar = outputValue;
      for (int i=3;i>=0;i--) {
	unsigned short thisValue=shortValue[i];
	// encode 6 bits at a time
	for (int j=0;j<3;j++) {
	  unsigned short thisPart = static_cast<unsigned short>(thisValue & 63);
	  thisValue = static_cast<unsigned short>(thisValue>>6);
	  if (thisPart < 10) {
	    *thisChar = static_cast<char>(thisPart+'0');
	  } else if (thisPart < 36) {
	    *thisChar = static_cast<char>(thisPart-10+'a');
	  } else if (thisPart < 62) {
	    *thisChar = static_cast<char>(thisPart-36+'A');
	  } else {
	    *thisChar = static_cast<char>(thisPart-62+'*');
	  }
	  thisChar++;
	}
      }
    } else {
      // not INTEL
      char * thisChar = outputValue;
      for (int i=0;i<4;i++) {
	unsigned short thisValue=shortValue[i];
	// encode 6 bits at a time
	for (int j=0;j<3;j++) {
	  unsigned short thisPart = static_cast<unsigned short>(thisValue & 63);
	  thisValue = static_cast<unsigned short>(thisValue>>6);
	  if (thisPart < 10) {
	    *thisChar = static_cast<char>(thisPart+'0');
	  } else if (thisPart < 36) {
	    *thisChar = static_cast<char>(thisPart-10+'a');
	  } else if (thisPart < 62) {
	    *thisChar = static_cast<char>(thisPart-36+'A');
	  } else {
	    *thisChar = static_cast<char>(thisPart-62+'*');
	  }
	  thisChar++;
	}
      }
    }
  }
}
static void
writeString(CoinFileOutput *output, const char* str)
{
   if (output != 0) {
      output->puts (str);
   }
}

// Put out card image
static void outputCard(int formatType,int numberFields,
		       CoinFileOutput *output,
		       std::string head, const char * name,
		       const char outputValue[2][24],
		       const char outputRow[2][100])
{
   // fprintf(fp,"%s",head.c_str());
   std::string line = head;
   int i;
   if (formatType==0||(formatType>=2&&formatType<8)) {
      char outputColumn[9];
      strcpy(outputColumn,name);
      for (i=0;i<8;i++) {
	 if (outputColumn[i]=='\0')
	    break;
      }
      for (;i<8;i++) 
	 outputColumn[i]=' ';
      outputColumn[8]='\0';
      // fprintf(fp,"%s  ",outputColumn);
      line += outputColumn;
      line += "  ";
      for (i=0;i<numberFields;i++) {
	 // fprintf(fp,"%s  %s",outputRow[i],outputValue[i]);
	 line += outputRow[i];
	 line += "  ";
	 line += outputValue[i];
	 if (i<numberFields-1) {
	    // fprintf(fp,"   ");
	    line += "   ";
	 }
      }
   } else {
      // fprintf(fp,"%s",name);
      line += name;
      for (i=0;i<numberFields;i++) {
	 // fprintf(fp," %s %s",outputRow[i],outputValue[i]);
	 line += " ";
	 line += outputRow[i];
	 line += " ";
	 line += outputValue[i];
      }
   }
   
   // fprintf(fp,"\n");
   line += "\n";
   writeString(output, line.c_str());
}
static int
makeUniqueNames(char ** names,int number,char first)
{
  int largest=-1;
  int i;
  for (i=0;i<number;i++) {
    char * name = names[i];
    if (name[0]==first&&strlen(name)==8) {
      // check number
      int n=0;
      for (int j=1;j<8;j++) {
        char num = name[j];
        if (num>='0'&&num<='9') {
          n *= 10;
          n += num-'0';
        } else {
          n=-1;
          break;
        }
      }
      if (n>=0)
        largest = CoinMax(largest,n);
    }
  }
  largest ++;
  if (largest>0) {
    // check
    char * used = new char[largest];
    memset(used,0,largest);
    int nDup=0;
    for (i=0;i<number;i++) {
      char * name = names[i];
      if (name[0]==first&&strlen(name)==8) {
        // check number
        int n=0;
        for (int j=1;j<8;j++) {
          char num = name[j];
          if (num>='0'&&num<='9') {
            n *= 10;
            n += num-'0';
          } else {
            n=-1;
            break;
          }
        }
        if (n>=0) {
          if (!used[n]) {
            used[n]=1;
          } else {
            // duplicate
            nDup++;
            free(names[i]);
            char newName[9];
            sprintf(newName,"%c%7.7d",first,largest);
            names[i] = CoinStrdup(newName);
            largest++;
          }
        }
      }
    }
    delete []used;
    return nDup;
  } else {
    return 0;
  }
}
static void
strcpyeq(char * output, const char * input)
{
  output[0]='=';
  strcpy(output+1,input);
}

int
CoinMpsIO::writeMps(const char *filename, int compression,
		   int formatType, int numberAcross,
		    CoinPackedMatrix * quadratic,
		    int numberSOS, const CoinSet * setInfo) const
{
  // Clean up format and numberacross
  numberAcross=CoinMax(1,numberAcross);
  numberAcross=CoinMin(2,numberAcross);
  formatType=CoinMax(0,formatType);
  formatType=CoinMin(2,formatType);
  int possibleCompression=0;
#ifdef COIN_HAS_ZLIB
  possibleCompression =1;
#endif
#ifdef COIN_HAS_BZLIB
  possibleCompression += 2;
#endif
  if ((compression&possibleCompression)==0) {
    // switch to other if possible
    if (compression&&possibleCompression)
      compression = 3-compression;
    else
      compression=0;
  }
  std::string line = filename;
   CoinFileOutput *output = 0;
   switch (compression) {
   case 1:
     if (strcmp(line.c_str() +(line.size()-3), ".gz") != 0) {
       line += ".gz";
     }
     output = CoinFileOutput::create (line, CoinFileOutput::COMPRESS_GZIP);
     break;

   case 2:
     if (strcmp(line.c_str() +(line.size()-4), ".bz2") != 0) {
       line += ".bz2";
     }
     output = CoinFileOutput::create (line, CoinFileOutput::COMPRESS_BZIP2);
     break;

   case 0:
   default:
     output = CoinFileOutput::create (line, CoinFileOutput::COMPRESS_NONE);
     break;
   }

   const char * const * const rowNames = names_[0];
   const char * const * const columnNames = names_[1];
   int i;
   unsigned int length = 8;
   bool freeFormat = (formatType==1);
   // Check names for uniqueness if default
   int nChanged;
   nChanged=makeUniqueNames(names_[0],numberRows_,'R');
   if (nChanged)
     handler_->message(COIN_MPS_CHANGED,messages_)<<"row"<<nChanged
                                                  <<CoinMessageEol;
   nChanged=makeUniqueNames(names_[1],numberColumns_,'C');
   if (nChanged)
     handler_->message(COIN_MPS_CHANGED,messages_)<<"column"<<nChanged
                                                  <<CoinMessageEol;
   for (i = 0 ; i < numberRows_; ++i) {
      if (strlen(rowNames[i]) > length) {
	 length = static_cast<int>(strlen(rowNames[i]));
	 break;
      }
   }
   if (length <= 8) {
      for (i = 0 ; i < numberColumns_; ++i) {
	 if (strlen(columnNames[i]) > length) {
	    length = static_cast<int>(strlen(columnNames[i]));
	    break;
	 }
      }
   }
   if (length > 8 && freeFormat!=1) {
      freeFormat = true;
      formatType += 8;
   }
   if (numberStringElements_) {
     freeFormat=true;
     numberAcross=1;
   }
   
   // NAME card

   line = "NAME          ";
   if (strcmp(problemName_,"")==0) {
      line.append("BLANK   ");
   } else {
      if (strlen(problemName_) >= 8) {
	 line.append(problemName_, 8);
      } else {
	 line.append(problemName_);
	 line.append(8-strlen(problemName_), ' ');
      }
   }
   if (freeFormat&&(formatType&7)!=2)
     line.append("  FREE");
   else if (freeFormat)
     line.append("  FREEIEEE");
   else if ((formatType&7)==2)
     line.append("  IEEE");
   // See if INTEL if IEEE
   if ((formatType&7)==2) {
     // test intel here and add 1 if not intel
     double value=1.0;
     char x[8];
     memcpy(x,&value,8);
     if (x[0]==63) {
       formatType ++; // not intel
     } else {
       assert (x[0]==0);
     }
   }
   // finish off name and do ROWS card and objective
   char* objrow =
     CoinStrdup(strcmp(objectiveName_,"")==0 ? "OBJROW" : objectiveName_);
   line.append("\nROWS\n N  ");
   line.append(objrow);
   line.append("\n");
   writeString(output, line.c_str());

   // Rows section
   // Sense array
   // But massage if looks odd
   char * sense = new char [numberRows_];
   memcpy( sense , getRowSense(), numberRows_);
   const double * rowLower = getRowLower();
   const double * rowUpper = getRowUpper();
  
   for (i=0;i<numberRows_;i++) {
      line = " ";
      if (sense[i]!='R') {
	 line.append(1,sense[i]);
      } else {
	if (rowLower[i]>-1.0e30) {
	  if(rowUpper[i]<1.0e30) {
	 line.append("L");
	  } else {
	    sense[i]='G';
	    line.append(1,sense[i]);
      }
	} else {
	  sense[i]='L';
	  line.append(1,sense[i]);
	}
      }
      line.append("  ");
      line.append(rowNames[i]);
      line.append("\n");
      writeString(output, line.c_str());
   }

   // COLUMNS card
   writeString(output, "COLUMNS\n");

   bool ifBounds=false;
   double largeValue = infinity_;
   largeValue = 1.0e30; // safer

   const double * columnLower = getColLower();
   const double * columnUpper = getColUpper();
   const double * objective = getObjCoefficients();
   const CoinPackedMatrix * matrix = getMatrixByCol();
   const double * elements = matrix->getElements();
   const int * rows = matrix->getIndices();
   const CoinBigIndex * starts = matrix->getVectorStarts();
   const int * lengths = matrix->getVectorLengths();

   char outputValue[2][24];
   char outputRow[2][100];
   // strings
   int nextRowString=numberRows_+10;
   int nextColumnString=numberColumns_+10;
   int whichString=0;
   const char * nextString=NULL;
   // mark string rows
   char * stringRow = new char[numberRows_+1];
   memset(stringRow,0,numberRows_+1);
   if (numberStringElements_) {
     decodeString(whichString,nextRowString,nextColumnString,nextString);
   }
   // Arrays so we can put out rows in order
   int * tempRow = new int [numberRows_];
   double * tempValue = new double [numberRows_];

   // Through columns (only put out if elements or objective value)
   for (i=0;i<numberColumns_;i++) {
     if (i==nextColumnString) {
       // set up
       int k=whichString;
       int iColumn=nextColumnString;
       int iRow=nextRowString;
       const char * dummy;
       while (iColumn==nextColumnString) {
	 stringRow[iRow]=1;
	 k++;
	 decodeString(k,iRow,iColumn,dummy);
       }
     }
     if (objective[i]||lengths[i]||i==nextColumnString) {
       // see if bound will be needed
       if (columnLower[i]||columnUpper[i]<largeValue||isInteger(i))
	 ifBounds=true;
       int numberFields=0;
       if (objective[i]) {
	 convertDouble(0,formatType,objective[i],outputValue[0],
		       objrow,outputRow[0]);
	 numberFields=1;
	 if (stringRow[numberRows_]) {
	   assert (objective[i]==STRING_VALUE);
	   assert (nextColumnString==i&&nextRowString==numberRows_);
	   strcpyeq(outputValue[0],nextString);
	   stringRow[numberRows_]=0;
	   decodeString(++whichString,nextRowString,nextColumnString,nextString);
	 }
       }
       if (numberFields==numberAcross) {
	 // put out card
	 outputCard(formatType, numberFields,
		    output, "    ",
		    columnNames[i],
		    outputValue,
		    outputRow);
	 numberFields=0;
       }
       int j;
       int numberEntries = lengths[i];
       int start = starts[i];
       for (j=0;j<numberEntries;j++) {
	 tempRow[j] = rows[start+j];
	 tempValue[j] = elements[start+j];
       }
       CoinSort_2(tempRow,tempRow+numberEntries,tempValue);
       for (j=0;j<numberEntries;j++) {
	 int jRow = tempRow[j];
	 double value = tempValue[j];
	 if (value&&!stringRow[jRow]) {
	   convertDouble(0,formatType,value,
			 outputValue[numberFields],
			 rowNames[jRow],
			 outputRow[numberFields]);
	   numberFields++;
	   if (numberFields==numberAcross) {
	     // put out card
	     outputCard(formatType, numberFields,
			output, "    ",
			columnNames[i],
			outputValue,
			outputRow);
	     numberFields=0;
	   }
	 }
       }
       if (numberFields) {
	 // put out card
	 outputCard(formatType, numberFields,
		    output, "    ",
		    columnNames[i],
		    outputValue,
		    outputRow);
       }
     }
     // end see if any strings
     if (i==nextColumnString) {
       int iColumn=nextColumnString;
       int iRow=nextRowString;
       while (iColumn==nextColumnString) {
	 double value = 1.0;
	 convertDouble(0,formatType,value,
		       outputValue[0],
		       rowNames[nextRowString],
		       outputRow[0]);
	 strcpyeq(outputValue[0],nextString);
	 // put out card
	 outputCard(formatType, 1,
		    output, "    ",
		    columnNames[i],
		    outputValue,
		    outputRow);
	 stringRow[iRow]=0;
	 decodeString(++whichString,nextRowString,nextColumnString,nextString);
       }
     }
   }
   delete [] tempRow;
   delete [] tempValue;
   delete [] stringRow;

   bool ifRange=false;
   // RHS
   writeString(output, "RHS\n");

   int numberFields = 0;
   // If there is any offset - then do that
   if (objectiveOffset_ ) {
     convertDouble(1,formatType,objectiveOffset_,
		   outputValue[0],
		   objrow,
		   outputRow[0]);
     numberFields++;
     if (numberFields==numberAcross) {
       // put out card
       outputCard(formatType, numberFields,
		  output, "    ",
		  "RHS",
		  outputValue,
		  outputRow);
       numberFields=0;
     }
   }
   for (i=0;i<numberRows_;i++) {
      double value;
      switch (sense[i]) {
      case 'E':
	 value=rowLower[i];
	 break;
      case 'R':
	 value=rowUpper[i];
	   ifRange=true;
	 break;
      case 'L':
	 value=rowUpper[i];
	 break;
      case 'G':
	 value=rowLower[i];
	 break;
      default:
	 value=0.0;
	 break;
      }
      if (value != 0.0) {
	 convertDouble(1,formatType,value,
		       outputValue[numberFields],
		       rowNames[i],
		       outputRow[numberFields]);
	 if (i==nextRowString&&nextColumnString>=numberColumns_) {
	   strcpyeq(outputValue[0],nextString);
	   decodeString(++whichString,nextRowString,nextColumnString,nextString);
	 }
	 numberFields++;
	 if (numberFields==numberAcross) {
	    // put out card
	    outputCard(formatType, numberFields,
		       output, "    ",
		       "RHS",
		       outputValue,
		       outputRow);
	    numberFields=0;
	 }
      }
   }
   if (numberFields) {
      // put out card
      outputCard(formatType, numberFields,
		 output, "    ",
		 "RHS",
		 outputValue,
		 outputRow);
   }

   if (ifRange) {
      // RANGES
      writeString(output, "RANGES\n");

      numberFields = 0;
      for (i=0;i<numberRows_;i++) {
	 if (sense[i]=='R') {
	    double value =rowUpper[i]-rowLower[i];
	    if (value<1.0e30) {
	      convertDouble(1,formatType,value,
			    outputValue[numberFields],
			    rowNames[i],
			    outputRow[numberFields]);
	      numberFields++;
	      if (numberFields==numberAcross) {
		// put out card
		outputCard(formatType, numberFields,
			   output, "    ",
			   "RANGE",
			   outputValue,
			   outputRow);
		numberFields=0;
	      }
	    }
	 }
      }
      if (numberFields) {
	 // put out card
	 outputCard(formatType, numberFields,
		    output, "    ",
		    "RANGE",
		    outputValue,
		    outputRow);
      }
   }
   delete [] sense;
   if (ifBounds) {
      // BOUNDS
      writeString(output, "BOUNDS\n");

      for (i=0;i<numberColumns_;i++) {
	if (i==nextColumnString) {
	  // just lo and up
	  if (columnLower[i]==STRING_VALUE) {
	    assert (nextRowString==numberRows_+1);
	    convertDouble(2,formatType,1.0,
			  outputValue[0],
			  columnNames[i],
			  outputRow[0]);
	    strcpyeq(outputValue[0],nextString);
	    decodeString(++whichString,nextRowString,nextColumnString,nextString);
	    if (i==nextColumnString) {
	      assert (columnUpper[i]==STRING_VALUE);
	      assert (nextRowString==numberRows_+2);
	      if (!strcmp(nextString,outputValue[0])) {
		// put out card FX
		outputCard(formatType, 1,
			   output, " FX ",
			   "BOUND",
			   outputValue,
			   outputRow);
	      } else {
		// put out card LO
		outputCard(formatType, 1,
			   output, " LO ",
			   "BOUND",
			   outputValue,
			   outputRow);
		// put out card UP
		strcpyeq(outputValue[0],nextString);
		outputCard(formatType, 1,
			   output, " UP ",
			   "BOUND",
			   outputValue,
			   outputRow);
	      }
	      decodeString(++whichString,nextRowString,nextColumnString,nextString);
	    } else {
	      // just LO
	      // put out card LO
	      outputCard(formatType, 1,
			 output, " LO ",
			 "BOUND",
			 outputValue,
			 outputRow);
	    }
	  } else if (columnUpper[i]==STRING_VALUE) {
	    assert (nextRowString==numberRows_+2);
	    convertDouble(2,formatType,1.0,
			  outputValue[0],
			  columnNames[i],
			  outputRow[0]);
	    strcpyeq(outputValue[0],nextString);
	    outputCard(formatType, 1,
		       output, " UP ",
		       "BOUND",
		       outputValue,
		       outputRow);
	    decodeString(++whichString,nextRowString,nextColumnString,nextString);
	  }
	  continue;
	}
	 if (objective[i]||lengths[i]) {
	    // see if bound will be needed
	    if (columnLower[i]||columnUpper[i]<largeValue||isInteger(i)) {
	      double lowerValue = columnLower[i];
	      double upperValue = columnUpper[i];
	      if (isInteger(i)) {
		// Old argument - what are correct ranges for integer variables
		lowerValue = CoinMax(lowerValue, -MAX_INTEGER);
		upperValue = CoinMin(upperValue, MAX_INTEGER);
	      }
	       int numberFields=1;
	       std::string header[2];
	       double value[2];
	       if (lowerValue<=-largeValue) {
		  // FR or MI
		  if (upperValue>=largeValue&&!isInteger(i)) {
		     header[0]=" FR ";
		     value[0] = largeValue;
		  } else {
		     header[0]=" MI ";
		     value[0] = -largeValue;
                     if (!isInteger(i))
                       header[1]=" UP ";
                     else
                       header[1]=" UI ";
                     if (upperValue<largeValue) 
                       value[1] = upperValue;
                     else
                       value[1] = largeValue;
		     numberFields=2;
		  }
	       } else if (fabs(upperValue-lowerValue)<1.0e-8) {
		  header[0]=" FX ";
		  value[0] = lowerValue;
	       } else {
		  // do LO if needed
		  if (lowerValue) {
		     // LO
		     header[0]=" LO ";
		     value[0] = lowerValue;
		     if (isInteger(i)) {
			// Integer variable so UI
			header[1]=" UI ";
                        if (upperValue<largeValue) 
                          value[1] = upperValue;
                        else
                          value[1] = largeValue;
			numberFields=2;
		     } else if (upperValue<largeValue) {
			// UP
			header[1]=" UP ";
			value[1] = upperValue;
			numberFields=2;
		     }
		  } else {
		     if (isInteger(i)) {
			// Integer variable so BV or UI
			if (fabs(upperValue-1.0)<1.0e-8) {
			   // BV
			   header[0]=" BV ";
			   value[0] = 1.0;
			} else {
			   // UI
			   header[0]=" UI ";
                           if (upperValue<largeValue) 
                             value[0] = upperValue;
                           else
                             value[0] = largeValue;
			}
		     } else {
			// UP
			header[0]=" UP ";
			value[0] = upperValue;
		     }
		  }
	       }
	       // put out fields
	       int j;
	       for (j=0;j<numberFields;j++) {
		  convertDouble(2,formatType,value[j],
				outputValue[0],
				columnNames[i],
				outputRow[0]);
		  // put out card
		  outputCard(formatType, 1,
			     output, header[j],
			     "BOUND",
			     outputValue,
			     outputRow);
	       }
	    }
	 }
      }
   }
   
   // do any quadratic part
   if (quadratic) {

     writeString(output, "QUADOBJ\n");

     const int * columnQuadratic = quadratic->getIndices();
     const CoinBigIndex * columnQuadraticStart = quadratic->getVectorStarts();
     const int * columnQuadraticLength = quadratic->getVectorLengths();
     const double * quadraticElement = quadratic->getElements();
     for (int iColumn=0;iColumn<numberColumns_;iColumn++) {
       int numberFields=0;
       for (int j=columnQuadraticStart[iColumn];
	    j<columnQuadraticStart[iColumn]+columnQuadraticLength[iColumn];j++) {
	 int jColumn = columnQuadratic[j];
	 double elementValue = quadraticElement[j];
	 convertDouble(0,formatType,elementValue,
		       outputValue[numberFields],
		       columnNames[jColumn],
		       outputRow[numberFields]);
	 numberFields++;
	 if (numberFields==numberAcross) {
	   // put out card
	   outputCard(formatType, numberFields,
		      output, "    ",
		      columnNames[iColumn],
		      outputValue,
		      outputRow);
	   numberFields=0;
	 }
       }
       if (numberFields) {
	 // put out card
	 outputCard(formatType, numberFields,
		    output, "    ",
		    columnNames[iColumn],
		    outputValue,
		    outputRow);
       }
     }
   }
   // SOS
   if (numberSOS) {
     writeString(output, "SOS\n");
     for (int i=0;i<numberSOS;i++) {
       int type = setInfo[i].setType();
       writeString(output, (type==1) ? " S1\n" : " S2\n");
       int n=setInfo[i].numberEntries();
       const int * which = setInfo[i].which();
       const double * weights = setInfo[i].weights();
       
       for (int j=0;j<n;j++) {
	 int k=which[j];
	 convertDouble(2,formatType,
		       weights ? weights[j] : COIN_DBL_MAX,outputValue[0],
		       "",outputRow[0]);
	 // put out card
	 outputCard(formatType, 1,
		    output, "   ",
		    columnNames[k],
		    outputValue,outputRow);
       }
     }
   }

   // and finish

   writeString(output, "ENDATA\n");

   free(objrow);

   delete output;
   return 0;
}
   
//------------------------------------------------------------------
// Problem name
const char * CoinMpsIO::getProblemName() const
{
  return problemName_;
}
// Objective name
const char * CoinMpsIO::getObjectiveName() const
{
  return objectiveName_;
}
// Rhs name
const char * CoinMpsIO::getRhsName() const
{
  return rhsName_;
}
// Range name
const char * CoinMpsIO::getRangeName() const
{
  return rangeName_;
}
// Bound name
const char * CoinMpsIO::getBoundName() const
{
  return boundName_;
}

//------------------------------------------------------------------
// Get number of rows, columns and elements
//------------------------------------------------------------------
int CoinMpsIO::getNumCols() const
{
  return numberColumns_;
}
int CoinMpsIO::getNumRows() const
{
  return numberRows_;
}
int CoinMpsIO::getNumElements() const
{
  return numberElements_;
}

//------------------------------------------------------------------
// Get pointer to column lower and upper bounds.
//------------------------------------------------------------------  
const double * CoinMpsIO::getColLower() const
{
  return collower_;
}
const double * CoinMpsIO::getColUpper() const
{
  return colupper_;
}

//------------------------------------------------------------------
// Get pointer to row lower and upper bounds.
//------------------------------------------------------------------  
const double * CoinMpsIO::getRowLower() const
{
  return rowlower_;
}
const double * CoinMpsIO::getRowUpper() const
{
  return rowupper_;
}
 
/** A quick inlined function to convert from lb/ub style constraint
    definition to sense/rhs/range style */
inline void
CoinMpsIO::convertBoundToSense(const double lower, const double upper,
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

//-----------------------------------------------------------------------------
/** A quick inlined function to convert from sense/rhs/range stryle constraint
    definition to lb/ub style */
inline void
CoinMpsIO::convertSenseToBound(const char sense, const double right,
					const double range,
					double& lower, double& upper) const
{
  switch (sense) {
  case 'E':
    lower = upper = right;
    break;
  case 'L':
    lower = -infinity_;
    upper = right;
    break;
  case 'G':
    lower = right;
    upper = infinity_;
    break;
  case 'R':
    lower = right - range;
    upper = right;
    break;
  case 'N':
    lower = -infinity_;
    upper = infinity_;
    break;
  }
}
//------------------------------------------------------------------
// Get sense of row constraints.
//------------------------------------------------------------------ 
const char * CoinMpsIO::getRowSense() const
{
  if ( rowsense_==NULL ) {

    int nr=numberRows_;
    rowsense_ = reinterpret_cast<char *> (malloc(nr*sizeof(char)));


    double dum1,dum2;
    int i;
    for ( i=0; i<nr; i++ ) {
      convertBoundToSense(rowlower_[i],rowupper_[i],rowsense_[i],dum1,dum2);
    }
  }
  return rowsense_;
}

//------------------------------------------------------------------
// Get the rhs of rows.
//------------------------------------------------------------------ 
const double * CoinMpsIO::getRightHandSide() const
{
  if ( rhs_==NULL ) {

    int nr=numberRows_;
    rhs_ = reinterpret_cast<double *> (malloc(nr*sizeof(double)));


    char dum1;
    double dum2;
    int i;
    for ( i=0; i<nr; i++ ) {
      convertBoundToSense(rowlower_[i],rowupper_[i],dum1,rhs_[i],dum2);
    }
  }
  return rhs_;
}

//------------------------------------------------------------------
// Get the range of rows.
// Length of returned vector is getNumRows();
//------------------------------------------------------------------ 
const double * CoinMpsIO::getRowRange() const
{
  if ( rowrange_==NULL ) {

    int nr=numberRows_;
    rowrange_ = reinterpret_cast<double *> (malloc(nr*sizeof(double)));
    std::fill(rowrange_,rowrange_+nr,0.0);

    char dum1;
    double dum2;
    int i;
    for ( i=0; i<nr; i++ ) {
      convertBoundToSense(rowlower_[i],rowupper_[i],dum1,dum2,rowrange_[i]);
    }
  }
  return rowrange_;
}

const double * CoinMpsIO::getObjCoefficients() const
{
  return objective_;
}
 
//------------------------------------------------------------------
// Create a row copy of the matrix ...
//------------------------------------------------------------------
const CoinPackedMatrix * CoinMpsIO::getMatrixByRow() const
{
  if ( matrixByRow_ == NULL && matrixByColumn_) {
    matrixByRow_ = new CoinPackedMatrix(*matrixByColumn_);
    matrixByRow_->reverseOrdering();
  }
  return matrixByRow_;
}

//------------------------------------------------------------------
// Create a column copy of the matrix ...
//------------------------------------------------------------------
const CoinPackedMatrix * CoinMpsIO::getMatrixByCol() const
{
  return matrixByColumn_;
}

//------------------------------------------------------------------
// Save the data ...
//------------------------------------------------------------------
void
CoinMpsIO::setMpsDataWithoutRowAndColNames(
                                  const CoinPackedMatrix& m, const double infinity,
                                  const double* collb, const double* colub,
                                  const double* obj, const char* integrality,
                                  const double* rowlb, const double* rowub)
{
  freeAll();
  if (m.isColOrdered()) {
    matrixByColumn_ = new CoinPackedMatrix(m);
  } else {
    matrixByColumn_ = new CoinPackedMatrix;
    matrixByColumn_->reverseOrderedCopyOf(m);
  }
  numberColumns_ = matrixByColumn_->getNumCols();
  numberRows_ = matrixByColumn_->getNumRows();
  numberElements_ = matrixByColumn_->getNumElements();
  defaultBound_ = 1;
  infinity_ = infinity;
  objectiveOffset_ = 0;
  
  rowlower_ = reinterpret_cast<double *> (malloc (numberRows_ * sizeof(double)));
  rowupper_ = reinterpret_cast<double *> (malloc (numberRows_ * sizeof(double)));
  collower_ = reinterpret_cast<double *> (malloc (numberColumns_ * sizeof(double)));
  colupper_ = reinterpret_cast<double *> (malloc (numberColumns_ * sizeof(double)));
  objective_ = reinterpret_cast<double *> (malloc (numberColumns_ * sizeof(double)));
  std::copy(rowlb, rowlb + numberRows_, rowlower_);
  std::copy(rowub, rowub + numberRows_, rowupper_);
  std::copy(collb, collb + numberColumns_, collower_);
  std::copy(colub, colub + numberColumns_, colupper_);
  std::copy(obj, obj + numberColumns_, objective_);
  if (integrality) {
    integerType_ = reinterpret_cast<char *> (malloc (numberColumns_ * sizeof(char)));
    std::copy(integrality, integrality + numberColumns_, integerType_);
  } else {
    integerType_ = NULL;
  }
    
  problemName_ = CoinStrdup("");
  objectiveName_ = CoinStrdup("");
  rhsName_ = CoinStrdup("");
  rangeName_ = CoinStrdup("");
  boundName_ = CoinStrdup("");
}


void
CoinMpsIO::setMpsDataColAndRowNames(
		      char const * const * const colnames,
		      char const * const * const rownames)
{
  releaseRowNames();
  releaseColumnNames();
   // If long names free format
  names_[0] = reinterpret_cast<char **> (malloc(numberRows_ * sizeof(char *)));
  names_[1] = reinterpret_cast<char **> (malloc (numberColumns_ * sizeof(char *)));
   numberHash_[0]=numberRows_;
   numberHash_[1]=numberColumns_;
   char** rowNames = names_[0];
   char** columnNames = names_[1];
   int i;
   if (rownames) {
     for (i = 0 ; i < numberRows_; ++i) {
       if (rownames[i]) {
         rowNames[i] = CoinStrdup(rownames[i]);
       } else {
         rowNames[i] = reinterpret_cast<char *> (malloc (9 * sizeof(char)));
         sprintf(rowNames[i],"R%7.7d",i);
       }
     }
   } else {
     for (i = 0; i < numberRows_; ++i) {
       rowNames[i] = reinterpret_cast<char *> (malloc (9 * sizeof(char)));
       sprintf(rowNames[i],"R%7.7d",i);
     }
   }
#ifndef NONAMES
   if (colnames) {
     for (i = 0 ; i < numberColumns_; ++i) {
       if (colnames[i]) {
         columnNames[i] = CoinStrdup(colnames[i]);
       } else {
         columnNames[i] = reinterpret_cast<char *> (malloc (9 * sizeof(char)));
         sprintf(columnNames[i],"C%7.7d",i);
       }
     }
   } else {
     for (i = 0; i < numberColumns_; ++i) {
       columnNames[i] = reinterpret_cast<char *> (malloc (9 * sizeof(char)));
       sprintf(columnNames[i],"C%7.7d",i);
     }
   }
#else
   const double * objective = getObjCoefficients();
   const CoinPackedMatrix * matrix = getMatrixByCol();
   const int * lengths = matrix->getVectorLengths();
   int k=0;
   for (i = 0 ; i < numberColumns_; ++i) {
     columnNames[i] = reinterpret_cast<char *> (malloc (9 * sizeof(char)));
     sprintf(columnNames[i],"C%7.7d",k);
     if (objective[i]||lengths[i])
       k++;
     }
#endif
}

void
CoinMpsIO::setMpsDataColAndRowNames(
		      const std::vector<std::string> & colnames,
		      const std::vector<std::string> & rownames)
{  
   // If long names free format
  names_[0] = reinterpret_cast<char **> (malloc(numberRows_ * sizeof(char *)));
  names_[1] = reinterpret_cast<char **> (malloc (numberColumns_ * sizeof(char *)));
   char** rowNames = names_[0];
   char** columnNames = names_[1];
   int i;
   if (rownames.size()!=0) {
     for (i = 0 ; i < numberRows_; ++i) {
       rowNames[i] = CoinStrdup(rownames[i].c_str());
     }
   } else {
     for (i = 0; i < numberRows_; ++i) {
       rowNames[i] = reinterpret_cast<char *> (malloc (9 * sizeof(char)));
       sprintf(rowNames[i],"R%7.7d",i);
     }
   }
   if (colnames.size()!=0) {
     for (i = 0 ; i < numberColumns_; ++i) {
       columnNames[i] = CoinStrdup(colnames[i].c_str());
     }
   } else {
     for (i = 0; i < numberColumns_; ++i) {
       columnNames[i] = reinterpret_cast<char *> (malloc (9 * sizeof(char)));
       sprintf(columnNames[i],"C%7.7d",i);
     }
   }
}

void
CoinMpsIO::setMpsData(const CoinPackedMatrix& m, const double infinity,
                      const double* collb, const double* colub,
                      const double* obj, const char* integrality,
                      const double* rowlb, const double* rowub,
                      char const * const * const colnames,
                      char const * const * const rownames)
{
  setMpsDataWithoutRowAndColNames(m,infinity,collb,colub,obj,integrality,rowlb,rowub);
  setMpsDataColAndRowNames(colnames,rownames);
}

void
CoinMpsIO::setMpsData(const CoinPackedMatrix& m, const double infinity,
                      const double* collb, const double* colub,
                      const double* obj, const char* integrality,
                      const double* rowlb, const double* rowub,
                      const std::vector<std::string> & colnames,
                      const std::vector<std::string> & rownames)
{
  setMpsDataWithoutRowAndColNames(m,infinity,collb,colub,obj,integrality,rowlb,rowub);
  setMpsDataColAndRowNames(colnames,rownames);
}

void
CoinMpsIO::setMpsData(const CoinPackedMatrix& m, const double infinity,
		      const double* collb, const double* colub,
		      const double* obj, const char* integrality,
		      const char* rowsen, const double* rowrhs,
		      const double* rowrng,
		      char const * const * const colnames,
		      char const * const * const rownames)
{
   const int numrows = m.getNumRows();

   double * rlb = numrows ? new double[numrows] : 0;
   double * rub = numrows ? new double[numrows] : 0;

   for (int i = 0; i < numrows; ++i) {
      convertSenseToBound(rowsen[i], rowrhs[i], rowrng[i], rlb[i], rub[i]);
   }
   setMpsData(m, infinity, collb, colub, obj, integrality, rlb, rub,
	      colnames, rownames);
   delete [] rlb;
   delete [] rub;
}

void
CoinMpsIO::setMpsData(const CoinPackedMatrix& m, const double infinity,
		      const double* collb, const double* colub,
		      const double* obj, const char* integrality,
		      const char* rowsen, const double* rowrhs,
		      const double* rowrng,
		      const std::vector<std::string> & colnames,
		      const std::vector<std::string> & rownames)
{
   const int numrows = m.getNumRows();

   double * rlb = numrows ? new double[numrows] : 0;
   double * rub = numrows ? new double[numrows] : 0;

   for (int i = 0; i < numrows; ++i) {
      convertSenseToBound(rowsen[i], rowrhs[i], rowrng[i], rlb[i], rub[i]);
   }
   setMpsData(m, infinity, collb, colub, obj, integrality, rlb, rub,
	      colnames, rownames);
   delete [] rlb;
   delete [] rub;
}

void
CoinMpsIO::setProblemName (const char *name)
{ free(problemName_) ;
  problemName_ = CoinStrdup(name) ; }

void
CoinMpsIO::setObjectiveName (const char *name)
{ free(objectiveName_) ;
  objectiveName_ = CoinStrdup(name) ; }

//------------------------------------------------------------------
// Return true if column is a continuous, binary, ...
//------------------------------------------------------------------
bool CoinMpsIO::isContinuous(int columnNumber) const
{
  const char * intType = integerType_;
  if ( intType==NULL ) return true;
  assert (columnNumber>=0 && columnNumber < numberColumns_);
  if ( intType[columnNumber]==0 ) return true;
  return false;
}

/* Return true if column is integer.
   Note: This function returns true if the the column
   is binary or a general integer.
*/
bool CoinMpsIO::isInteger(int columnNumber) const
{
  const char * intType = integerType_;
  if ( intType==NULL ) return false;
  assert (columnNumber>=0 && columnNumber < numberColumns_);
  if ( intType[columnNumber]!=0 ) return true;
  return false;
}
// if integer
const char * CoinMpsIO::integerColumns() const
{
  return integerType_;
}
// Pass in array saying if each variable integer
void 
CoinMpsIO::copyInIntegerInformation(const char * integerType)
{
  if (integerType) {
    if (!integerType_)
      integerType_ = reinterpret_cast<char *> (malloc (numberColumns_ * sizeof(char)));
    memcpy(integerType_,integerType,numberColumns_);
  } else {
    free(integerType_);
    integerType_=NULL;
  }
}
// names - returns NULL if out of range
const char * CoinMpsIO::rowName(int index) const
{
  if (index>=0&&index<numberRows_) {
    return names_[0][index];
  } else {
    return NULL;
  }
}
const char * CoinMpsIO::columnName(int index) const
{
  if (index>=0&&index<numberColumns_) {
    return names_[1][index];
  } else {
    return NULL;
  }
}
// names - returns -1 if name not found
int CoinMpsIO::rowIndex(const char * name) const
{
  if (!hash_[0]) {
    if (numberRows_) {
      startHash(0);
    } else {
      return -1;
    }
  }
  return findHash ( name , 0 );
}
    int CoinMpsIO::columnIndex(const char * name) const
{
  if (!hash_[1]) {
    if (numberColumns_) {
      startHash(1);
    } else {
      return -1;
    }
  }
  return findHash ( name , 1 );
}

// Release all row information (lower, upper)
void CoinMpsIO::releaseRowInformation()
{
  free(rowlower_);
  free(rowupper_);
  rowlower_=NULL;
  rowupper_=NULL;
}
// Release all column information (lower, upper, objective)
void CoinMpsIO::releaseColumnInformation()
{
  free(collower_);
  free(colupper_);
  free(objective_);
  collower_=NULL;
  colupper_=NULL;
  objective_=NULL;
}
// Release integer information
void CoinMpsIO::releaseIntegerInformation()
{
  free(integerType_);
  integerType_=NULL;
}
// Release row names
void CoinMpsIO::releaseRowNames()
{
  releaseRedundantInformation();
  int i;
  for (i=0;i<numberHash_[0];i++) {
    free(names_[0][i]);
  }
  free(names_[0]);
  names_[0]=NULL;
  numberHash_[0]=0;
}
// Release column names
void CoinMpsIO::releaseColumnNames()
{
  releaseRedundantInformation();
  int i;
  for (i=0;i<numberHash_[1];i++) {
    free(names_[1][i]);
  }
  free(names_[1]);
  names_[1]=NULL;
  numberHash_[1]=0;
}
// Release matrix information
void CoinMpsIO::releaseMatrixInformation()
{
  releaseRedundantInformation();
  delete matrixByColumn_;
  matrixByColumn_=NULL;
}
  


//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
CoinMpsIO::CoinMpsIO ()
:
problemName_(CoinStrdup("")),
objectiveName_(CoinStrdup("")),
rhsName_(CoinStrdup("")),
rangeName_(CoinStrdup("")),
boundName_(CoinStrdup("")),
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
fileName_(CoinStrdup("????")),
defaultBound_(1),
infinity_(COIN_DBL_MAX),
smallElement_(1.0e-14),
defaultHandler_(true),
cardReader_(NULL),
convertObjective_(false),
allowStringElements_(0),
maximumStringElements_(0),
numberStringElements_(0),
stringElements_(NULL)
{
  numberHash_[0]=0;
  hash_[0]=NULL;
  names_[0]=NULL;
  numberHash_[1]=0;
  hash_[1]=NULL;
  names_[1]=NULL;
  handler_ = new CoinMessageHandler();
  messages_ = CoinMessage();
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
CoinMpsIO::CoinMpsIO(const CoinMpsIO & rhs)
:
problemName_(CoinStrdup("")),
objectiveName_(CoinStrdup("")),
rhsName_(CoinStrdup("")),
rangeName_(CoinStrdup("")),
boundName_(CoinStrdup("")),
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
fileName_(CoinStrdup("????")),
defaultBound_(1),
infinity_(COIN_DBL_MAX),
smallElement_(1.0e-14),
defaultHandler_(true),
cardReader_(NULL),
allowStringElements_(rhs.allowStringElements_),
maximumStringElements_(rhs.maximumStringElements_),
numberStringElements_(rhs.numberStringElements_),
stringElements_(NULL)
{
  numberHash_[0]=0;
  hash_[0]=NULL;
  names_[0]=NULL;
  numberHash_[1]=0;
  hash_[1]=NULL;
  names_[1]=NULL;
  if ( rhs.rowlower_ !=NULL || rhs.collower_ != NULL) {
    gutsOfCopy(rhs);
    // OK and proper to leave rowsense_, rhs_, and
    // rowrange_ (also row copy and hash) to NULL.  They will be constructed
    // if they are required.
  }
  defaultHandler_ = rhs.defaultHandler_;
  if (defaultHandler_)
    handler_ = new CoinMessageHandler(*rhs.handler_);
  else
    handler_ = rhs.handler_;
  messages_ = CoinMessage();
}

void CoinMpsIO::gutsOfCopy(const CoinMpsIO & rhs)
{
  defaultHandler_ = rhs.defaultHandler_;
  if (rhs.matrixByColumn_)
    matrixByColumn_=new CoinPackedMatrix(*(rhs.matrixByColumn_));
  numberElements_=rhs.numberElements_;
  numberRows_=rhs.numberRows_;
  numberColumns_=rhs.numberColumns_;
  convertObjective_=rhs.convertObjective_;
  if (rhs.rowlower_) {
    rowlower_ = reinterpret_cast<double *> (malloc(numberRows_*sizeof(double)));
    rowupper_ = reinterpret_cast<double *> (malloc(numberRows_*sizeof(double)));
    memcpy(rowlower_,rhs.rowlower_,numberRows_*sizeof(double));
    memcpy(rowupper_,rhs.rowupper_,numberRows_*sizeof(double));
  }
  if (rhs.collower_) {
    collower_ = reinterpret_cast<double *> (malloc(numberColumns_*sizeof(double)));
    colupper_ = reinterpret_cast<double *> (malloc(numberColumns_*sizeof(double)));
    objective_ = reinterpret_cast<double *> (malloc(numberColumns_*sizeof(double)));
    memcpy(collower_,rhs.collower_,numberColumns_*sizeof(double));
    memcpy(colupper_,rhs.colupper_,numberColumns_*sizeof(double));
    memcpy(objective_,rhs.objective_,numberColumns_*sizeof(double));
  }
  if (rhs.integerType_) {
    integerType_ = reinterpret_cast<char *> (malloc (numberColumns_*sizeof(char)));
    memcpy(integerType_,rhs.integerType_,numberColumns_*sizeof(char));
  }
  free(fileName_);
  free(problemName_);
  free(objectiveName_);
  free(rhsName_);
  free(rangeName_);
  free(boundName_);
  fileName_ = CoinStrdup(rhs.fileName_);
  problemName_ = CoinStrdup(rhs.problemName_);
  objectiveName_ = CoinStrdup(rhs.objectiveName_);
  rhsName_ = CoinStrdup(rhs.rhsName_);
  rangeName_ = CoinStrdup(rhs.rangeName_);
  boundName_ = CoinStrdup(rhs.boundName_);
  numberHash_[0]=rhs.numberHash_[0];
  numberHash_[1]=rhs.numberHash_[1];
  defaultBound_=rhs.defaultBound_;
  infinity_=rhs.infinity_;
  smallElement_ = rhs.smallElement_;
  objectiveOffset_=rhs.objectiveOffset_;
  int section;
  for (section=0;section<2;section++) {
    if (numberHash_[section]) {
      char ** names2 = rhs.names_[section];
      names_[section] = reinterpret_cast<char **> (malloc(numberHash_[section]*
						     sizeof(char *)));
      char ** names = names_[section];
      int i;
      for (i=0;i<numberHash_[section];i++) {
	names[i]=CoinStrdup(names2[i]);
      }
    }
  }
  allowStringElements_ = rhs.allowStringElements_;
  maximumStringElements_ = rhs.maximumStringElements_;
  numberStringElements_ = rhs.numberStringElements_;
  if (numberStringElements_) {
    stringElements_ = new char * [maximumStringElements_];
    for (int i=0;i<numberStringElements_;i++)
      stringElements_[i]=CoinStrdup(rhs.stringElements_[i]);
  } else {
    stringElements_ = NULL;
  }
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
CoinMpsIO::~CoinMpsIO ()
{
  gutsOfDestructor();
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
CoinMpsIO &
CoinMpsIO::operator=(const CoinMpsIO& rhs)
{
  if (this != &rhs) {    
    gutsOfDestructor();
    if ( rhs.rowlower_ !=NULL || rhs.collower_ != NULL) {
      gutsOfCopy(rhs);
    }
    defaultHandler_ = rhs.defaultHandler_;
    if (defaultHandler_)
      handler_ = new CoinMessageHandler(*rhs.handler_);
    else
      handler_ = rhs.handler_;
    messages_ = CoinMessage();
  }
  return *this;
}

//-------------------------------------------------------------------
void CoinMpsIO::gutsOfDestructor()
{  
  freeAll();
  if (defaultHandler_) {
    delete handler_;
    handler_ = NULL;
  }
  delete cardReader_;
  cardReader_ = NULL;
}


void CoinMpsIO::freeAll()
{  
  releaseRedundantInformation();
  releaseRowNames();
  releaseColumnNames();
  delete matrixByRow_;
  delete matrixByColumn_;
  matrixByRow_=NULL;
  matrixByColumn_=NULL;
  free(rowlower_);
  free(rowupper_);
  free(collower_);
  free(colupper_);
  free(objective_);
  free(integerType_);
  free(fileName_);
  rowlower_=NULL;
  rowupper_=NULL;
  collower_=NULL;
  colupper_=NULL;
  objective_=NULL;
  integerType_=NULL;
  fileName_=NULL;
  free(problemName_);
  free(objectiveName_);
  free(rhsName_);
  free(rangeName_);
  free(boundName_);
  problemName_=NULL;
  objectiveName_=NULL;
  rhsName_=NULL;
  rangeName_=NULL;
  boundName_=NULL;
  for (int i=0;i<numberStringElements_;i++)
    free(stringElements_[i]);
  delete [] stringElements_;
}

/* Release all information which can be re-calculated e.g. rowsense
    also any row copies OR hash tables for names */
void CoinMpsIO::releaseRedundantInformation()
{  
  free( rowsense_);
  free( rhs_);
  free( rowrange_);
  rowsense_=NULL;
  rhs_=NULL;
  rowrange_=NULL;
  delete [] hash_[0];
  delete [] hash_[1];
  hash_[0]=0;
  hash_[1]=0;
  delete matrixByRow_;
  matrixByRow_=NULL;
}
// Pass in Message handler (not deleted at end)
void 
CoinMpsIO::passInMessageHandler(CoinMessageHandler * handler)
{
  if (defaultHandler_) 
    delete handler_;
  defaultHandler_=false;
  handler_=handler;
}
// Set language
void 
CoinMpsIO::newLanguage(CoinMessages::Language language)
{
  messages_ = CoinMessage(language);
}

/* Read in a quadratic objective from the given filename.  
   If filename is NULL then continues reading from previous file.  If
   not then the previous file is closed.
   
   No assumption is made on symmetry, positive definite etc.
   No check is made for duplicates or non-triangular
   
   Returns number of errors
*/
int 
CoinMpsIO::readQuadraticMps(const char * filename,
			    int * &columnStart, int * &column2, double * &elements,
			    int checkSymmetry)
{
  // Deal with filename - +1 if new, 0 if same as before, -1 if error
  CoinFileInput *input = 0;
  int returnCode = dealWithFileName(filename,"",input);
  if (returnCode<0) {
    return -1;
  } else if (returnCode>0) {
    delete cardReader_;
    cardReader_ = new CoinMpsCardReader ( input, this);
  }
  // See if QUADOBJ just found
  if (!filename&&cardReader_->whichSection (  ) == COIN_QUAD_SECTION ) {
    cardReader_->setWhichSection(COIN_QUADRATIC_SECTION);
  } else {
    cardReader_->readToNextSection();
    
    // Skip NAME
    if ( cardReader_->whichSection (  ) == COIN_NAME_SECTION ) 
      cardReader_->readToNextSection();
    if ( cardReader_->whichSection (  ) == COIN_QUADRATIC_SECTION ) {
      // save name of section
      free(problemName_);
      problemName_=CoinStrdup(cardReader_->columnName());
    } else if ( cardReader_->whichSection (  ) == COIN_EOF_SECTION ) {
      handler_->message(COIN_MPS_EOF,messages_)<<fileName_
					       <<CoinMessageEol;
      return -3;
    } else {
    handler_->message(COIN_MPS_BADFILE1,messages_)<<cardReader_->card()
						  <<cardReader_->cardNumber()
						  <<fileName_
						  <<CoinMessageEol;
    return -2;
    }
  }    

  int numberErrors = 0;

  // Guess at size of data
  int maximumNonZeros = 5 *numberColumns_;
  // Use malloc so can use realloc
  int * column = reinterpret_cast<int *> (malloc(maximumNonZeros*sizeof(int)));
  int * column2Temp = reinterpret_cast<int *> (malloc(maximumNonZeros*sizeof(int)));
  double * elementTemp = reinterpret_cast<double *> (malloc(maximumNonZeros*sizeof(double)));

  startHash(1);
  int numberElements=0;

  while ( cardReader_->nextField (  ) == COIN_QUADRATIC_SECTION ) {
    switch ( cardReader_->mpsType (  ) ) {
    case COIN_BLANK_COLUMN:
      if ( fabs ( cardReader_->value (  ) ) > smallElement_ ) {
	if ( numberElements == maximumNonZeros ) {
	  maximumNonZeros = ( 3 * maximumNonZeros ) / 2 + 1000;
	  column = reinterpret_cast<COINColumnIndex * >
	    (realloc ( column, maximumNonZeros * sizeof ( COINColumnIndex )));
	  column2Temp = reinterpret_cast<COINColumnIndex *>
	    (realloc ( column2Temp, maximumNonZeros * sizeof ( COINColumnIndex )));
	  elementTemp = reinterpret_cast<double *>
	    (realloc ( elementTemp, maximumNonZeros * sizeof ( double )));
	}
	// get indices
	COINColumnIndex iColumn1 = findHash ( cardReader_->columnName (  ) , 1 );
	COINColumnIndex iColumn2 = findHash ( cardReader_->rowName (  ) , 1 );

	if ( iColumn1 >= 0 ) {
	  if (iColumn2 >=0) {
	    double value = cardReader_->value (  );
	    column[numberElements]=iColumn1;
	    column2Temp[numberElements]=iColumn2;
	    elementTemp[numberElements++]=value;
	  } else {
	    numberErrors++;
	    if ( numberErrors < 100 ) {
		  handler_->message(COIN_MPS_NOMATCHROW,messages_)
		    <<cardReader_->rowName()<<cardReader_->cardNumber()<<cardReader_->card()
		    <<CoinMessageEol;
	    } else if (numberErrors > 100000) {
	      handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	      return numberErrors;
	    }
	  }
	} else {
	  numberErrors++;
	  if ( numberErrors < 100 ) {
	    handler_->message(COIN_MPS_NOMATCHCOL,messages_)
	      <<cardReader_->columnName()<<cardReader_->cardNumber()<<cardReader_->card()
	      <<CoinMessageEol;
	  } else if (numberErrors > 100000) {
	    handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	    return numberErrors;
	  }
	}
      }
      break;
    default:
      numberErrors++;
      if ( numberErrors < 100 ) {
	handler_->message(COIN_MPS_BADIMAGE,messages_)<<cardReader_->cardNumber()
						      <<cardReader_->card()
						      <<CoinMessageEol;
      } else if (numberErrors > 100000) {
	handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	return numberErrors;
      }
    }
  }
  stopHash(1);
  // Do arrays as new [] and make column ordered
  columnStart = new int [numberColumns_+1];
  // for counts
  int * count = new int[numberColumns_];
  memset(count,0,numberColumns_*sizeof(int));
  CoinBigIndex i;
  // See about lower triangular
  if (checkSymmetry&&numberErrors) 
    checkSymmetry=2; // force corrections
  if (checkSymmetry) {
    if (checkSymmetry==1) {
      // just check lower triangular
      for ( i = 0; i < numberElements; i++ ) {
	int iColumn = column[i];
	int iColumn2 = column2Temp[i];
	if (iColumn2<iColumn) {
	  numberErrors=-4;
	  column[i]=iColumn2;
	  column2Temp[i]=iColumn;
	}
      }
    } else {
      // make lower triangular
      for ( i = 0; i < numberElements; i++ ) {
	int iColumn = column[i];
	int iColumn2 = column2Temp[i];
	if (iColumn2<iColumn) {
	  column[i]=iColumn2;
	  column2Temp[i]=iColumn;
	}
      }
    }
  }
  for ( i = 0; i < numberElements; i++ ) {
    int iColumn = column[i];
    count[iColumn]++;
  }
  // Do starts
  int number = 0;
  columnStart[0]=0;
  for (i=0;i<numberColumns_;i++) {
    number += count[i];
    count[i]= columnStart[i];
    columnStart[i+1]=number;
  }
  column2 = new int[numberElements];
  elements = new double[numberElements];

  // Get column ordering
  for ( i = 0; i < numberElements; i++ ) {
    int iColumn = column[i];
    int iColumn2 = column2Temp[i];
    int put = count[iColumn];
    elements[put]=elementTemp[i];
    column2[put++]=iColumn2;
    count[iColumn]=put;
  }
  free(column);
  free(column2Temp);
  free(elementTemp);

  // Now in column order - deal with duplicates
  for (i=0;i<numberColumns_;i++) 
    count[i] = -1;

  int start = 0;
  number=0;
  for (i=0;i<numberColumns_;i++) {
    int j;
    for (j=start;j<columnStart[i+1];j++) {
      int iColumn2 = column2[j];
      if (count[iColumn2]<0) {
	count[iColumn2]=j;
      } else {
	// duplicate
	int iOther = count[iColumn2];
	double value = elements[iOther]+elements[j];
	elements[iOther]=value;
	elements[j]=0.0;
      }
    }
    for (j=start;j<columnStart[i+1];j++) {
      int iColumn2 = column2[j];
      count[iColumn2]=-1;
      double value = elements[j];
      if (value) {
	column2[number]=iColumn2;
	elements[number++]=value;
      }
    }
    start = columnStart[i+1];
    columnStart[i+1]=number;
  }

  delete [] count;
  return numberErrors;
}
/* Read in a list of cones from the given filename.  
   If filename is NULL (or same) then continues reading from previous file.
   If not then the previous file is closed.  Code should be added to
   general MPS reader to read this if CSECTION
   
   No checking is done that in unique cone
   
   Arrays should be deleted by delete []
   
   Returns number of errors, -1 bad file, -2 no conic section, -3 empty section
   
   columnStart is numberCones+1 long, other number of columns in matrix
*/
int 
CoinMpsIO::readConicMps(const char * filename,
		     int * &columnStart, int * &column, int & numberCones)
{
  // Deal with filename - +1 if new, 0 if same as before, -1 if error
  CoinFileInput *input = 0;
  int returnCode = dealWithFileName(filename,"",input);
  if (returnCode<0) {
    return -1;
  } else if (returnCode>0) {
    delete cardReader_;
    cardReader_ = new CoinMpsCardReader ( input, this);
  }

  cardReader_->readToNextSection();

  // Skip NAME
  if ( cardReader_->whichSection (  ) == COIN_NAME_SECTION ) 
    cardReader_->readToNextSection();
  numberCones=0;

  // Get arrays
  columnStart = new int [numberColumns_+1];
  column = new int [numberColumns_];
  int numberErrors = 0;
  columnStart[0]=0;
  int numberElements=0;
  startHash(1);
  
  //if (cardReader_->whichSection()==COIN_CONIC_SECTION) 
  //cardReader_->cleanCard(); // skip doing last
  while ( cardReader_->nextField (  ) == COIN_CONIC_SECTION ) {
    // should check QUAD
    // Have to check by hand
    if (!strncmp(cardReader_->card(),"CSECTION",8)) {
      if (numberElements==columnStart[numberCones]) {
	printf("Cone must have at least one column\n");
	abort();
      }
      columnStart[++numberCones]=numberElements;
      continue;
    }
    COINColumnIndex iColumn1;
    switch ( cardReader_->mpsType (  ) ) {
    case COIN_BLANK_COLUMN:
      // get index
      iColumn1 = findHash ( cardReader_->columnName (  ) , 1 );
      
      if ( iColumn1 >= 0 ) {
	column[numberElements++]=iColumn1;
      } else {
	numberErrors++;
	if ( numberErrors < 100 ) {
	  handler_->message(COIN_MPS_NOMATCHCOL,messages_)
	    <<cardReader_->columnName()<<cardReader_->cardNumber()<<cardReader_->card()
	    <<CoinMessageEol;
	} else if (numberErrors > 100000) {
	  handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	  return numberErrors;
	}
      }
      break;
    default:
      numberErrors++;
      if ( numberErrors < 100 ) {
	handler_->message(COIN_MPS_BADIMAGE,messages_)<<cardReader_->cardNumber()
						      <<cardReader_->card()
						      <<CoinMessageEol;
      } else if (numberErrors > 100000) {
	handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	return numberErrors;
      }
    }
  }
  if ( cardReader_->whichSection (  ) == COIN_ENDATA_SECTION ) {
    // Error if no cones
    if (!numberElements) {
      handler_->message(COIN_MPS_EOF,messages_)<<fileName_
					       <<CoinMessageEol;
      delete [] columnStart;
      delete [] column;
      columnStart = NULL;
      column = NULL;
      return -3;
    } else {
      columnStart[++numberCones]=numberElements;
    }
  } else {
    handler_->message(COIN_MPS_BADFILE1,messages_)<<cardReader_->card()
						  <<cardReader_->cardNumber()
						 <<fileName_
						  <<CoinMessageEol;
    delete [] columnStart;
    delete [] column;
    columnStart = NULL;
    column = NULL;
    numberCones=0;
    return -2;
  }

  stopHash(1);
  return numberErrors;
}
// Add string to list
void 
CoinMpsIO::addString(int iRow,int iColumn, const char * value)
{
  char id [20];
  sprintf(id,"%d,%d,",iRow,iColumn);
  int n = static_cast<int>(strlen(id)+strlen(value));
  if (numberStringElements_==maximumStringElements_) {
    maximumStringElements_ = 2*maximumStringElements_+100;
    char ** temp = new char * [maximumStringElements_];
    for (int i=0;i<numberStringElements_;i++)
      temp[i]=stringElements_[i];
    delete [] stringElements_;
    stringElements_ = temp;
  }
  char * line = reinterpret_cast<char *> (malloc(n+1));
  stringElements_[numberStringElements_++]=line;
  strcpy(line,id);
  strcat(line,value);
}
// Decode string
void 
CoinMpsIO::decodeString(int iString, int & iRow, int & iColumn, const char * & value) const
{
  iRow=-1;
  iColumn=-1;
  value=NULL;
  if (iString>=0&&iString<numberStringElements_) {
    value = stringElements_[iString];
    sscanf(value,"%d,%d,",&iRow,&iColumn);
    value = strchr(value,',');
    assert(value);
    value++;
    value = strchr(value,',');
    assert(value);
    value++;
  }
}
// copies in strings from a CoinModel - returns number
int 
CoinMpsIO::copyStringElements(const CoinModel * model)
{
  if (!model->stringsExist())
    return 0; // no strings
  assert (!numberStringElements_);
  /*
    First columns (including objective==numberRows)
    then RHS(==numberColumns (+1)) (with rowLower and rowUpper marked)
    then bounds LO==numberRows+1, UP==numberRows+2
  */
  int numberColumns = model->numberColumns();
  int numberRows = model->numberRows();
  int iColumn;
  for (iColumn=0;iColumn<numberColumns;iColumn++) {
    const char * expr = model->getColumnObjectiveAsString(iColumn);
    if (strcmp(expr,"Numeric")) {
      addString(numberRows,iColumn,expr);
    }
    CoinModelLink triple=model->firstInColumn(iColumn);
    while (triple.row()>=0) {
      int iRow = triple.row();
      const char * expr = model->getElementAsString(iRow,iColumn);
      if (strcmp(expr,"Numeric")) {
	addString(iRow,iColumn,expr);
      }
      triple=model->next(triple);
    }
  }
  int iRow;
  for (iRow=0;iRow<numberRows;iRow++) {
    // for now no ranges
    const char * expr1 = model->getRowLowerAsString(iRow);
    const char * expr2 = model->getRowUpperAsString(iRow);
    if (strcmp(expr1,"Numeric")) {
      if (rowupper_[iRow]>1.0e20&&!strcmp(expr2,"Numeric")) {
	// G row
	addString(iRow,numberColumns,expr1);
	rowlower_[iRow]=STRING_VALUE;
      } else if (!strcmp(expr1,expr2)) {
	// E row
	addString(iRow,numberColumns,expr1);
	rowlower_[iRow]=STRING_VALUE;
	addString(iRow,numberColumns+1,expr1);
	rowupper_[iRow]=STRING_VALUE;
      } else if (rowlower_[iRow]<-1.0e20&&!strcmp(expr1,"Numeric")) {
	// L row
	addString(iRow,numberColumns+1,expr2);
	rowupper_[iRow]=STRING_VALUE;
      } else {
	// Range
	printf("Unaable to handle string ranges row %d %s %s\n",
	       iRow,expr1,expr2);
	abort();
      }
    }
  }
  // Bounds
  for (iColumn=0;iColumn<numberColumns;iColumn++) {
    const char * expr = model->getColumnLowerAsString(iColumn);
    if (strcmp(expr,"Numeric")) {
      addString(numberRows+1,iColumn,expr);
      collower_[iColumn]=STRING_VALUE;
    }
    expr = model->getColumnUpperAsString(iColumn);
    if (strcmp(expr,"Numeric")) {
      addString(numberRows+2,iColumn,expr);
      colupper_[iColumn]=STRING_VALUE;
    }
  }
  return numberStringElements_;
}
// Constructor 
CoinSet::CoinSet ( int numberEntries, const int * which)
{
  numberEntries_ = numberEntries;
  which_ = new int [numberEntries_];
  weights_ = NULL;
  memcpy(which_,which,numberEntries_*sizeof(int));
  setType_=1;
}
// Default constructor 
CoinSet::CoinSet ()
{
  numberEntries_ = 0;
  which_ = NULL;
  weights_ = NULL;
  setType_=1;
}

// Copy constructor 
CoinSet::CoinSet (const CoinSet & rhs)
{
  numberEntries_ = rhs.numberEntries_;
  setType_=rhs.setType_;
  which_ = CoinCopyOfArray(rhs.which_,numberEntries_);
  weights_ = CoinCopyOfArray(rhs.weights_,numberEntries_);
}
  
//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
CoinSet &
CoinSet::operator=(const CoinSet& rhs)
{
  if (this != &rhs) {    
    delete [] which_;
    delete [] weights_;
    numberEntries_ = rhs.numberEntries_;
    setType_=rhs.setType_;
    which_ = CoinCopyOfArray(rhs.which_,numberEntries_);
    weights_ = CoinCopyOfArray(rhs.weights_,numberEntries_);
  }
  return *this;
}

// Destructor
CoinSet::~CoinSet (  )
{
  delete [] which_;
  delete [] weights_;
}
// Constructor 
CoinSosSet::CoinSosSet ( int numberEntries, const int * which, const double * weights, int type)
  : CoinSet(numberEntries,which)
{
  weights_= new double [numberEntries_];
  memcpy(weights_,weights,numberEntries_*sizeof(double));
  setType_ = type;
  double last = weights_[0];
  int i;
  bool allSame=true;
  for (i=1;i<numberEntries_;i++) {
    if(weights_[i]!=last) {
      allSame=false;
      break;
    }
  }
  if (allSame) {
    for (i=0;i<numberEntries_;i++) 
      weights_[i] = i;
  }
}

// Destructor
CoinSosSet::~CoinSosSet (  )
{
}
#ifdef USE_SBB
#include "SbbModel.hpp"
#include "SbbBranchActual.hpp"
// returns an object of type SbbObject
SbbObject * 
CoinSosSet::sbbObject(SbbModel * model) const 
{
  // which are matrix here - need to put as integer index
  abort();
  return new SbbSOS(model,numberEntries_,which_,weights_,0,setType_);
}
#endif
