// Copyright (C) 2005, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>

#include "CoinMpsIO.hpp"
#include "CoinModel.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinTime.hpp"

//#############################################################################

/* Build a model in some interesting way
   
   Nine blocks defined by 4 random numbers
   If next random number <0.4 addRow next possible
   If <0.8 addColumn if possible
   Otherwise do by element
   For each one use random <0.5 to add rim at same time
   At end fill in rim at random

*/
void buildRandom(CoinModel & baseModel, double random, double & timeIt, int iPass)
{
  CoinModel model;
  int numberRows = baseModel.numberRows();
  int numberColumns = baseModel.numberColumns();
  int numberElements = baseModel.numberElements();
  // Row numbers and column numbers
  int lastRow[4];
  int lastColumn[4];
  int i;
  lastRow[0]=0;
  lastColumn[0]=0;
  lastRow[3]=numberRows;
  lastColumn[3]=numberColumns;
  for ( i=1;i<3;i++) {
    int size;
    size = (int) ((0.25*random+0.2)*numberRows);
    random = CoinDrand48();
    lastRow[i]=lastRow[i-1]+size;
    size = (int) ((0.25*random+0.2)*numberColumns);
    random = CoinDrand48();
    lastColumn[i]=lastColumn[i-1]+size;
  }
  // whether block done 0 row first
  char block[9]={0,0,0,0,0,0,0,0,0};
  // whether rowRim done
  char rowDone[3]={0,0,0};
  // whether columnRim done
  char columnDone[3]={0,0,0};
  // Save array for deleting elements
  CoinModelTriple * dTriple = new CoinModelTriple[numberElements];
  numberElements=0;
  double time1 = CoinCpuTime();
  for (int jBlock=0;jBlock<9;jBlock++) {
    int iType=-1;
    int iBlock=-1;
    if (random<0.4) {
      // trying addRow
      int j;
      for (j=0;j<3;j++) {
        int k;
        for (k=0;k<3;k++) {
          if (block[3*j+k])
            break; //already taken
        }
        if (k==3) {
          // can do but decide which one
          iBlock = 3*j + (int) (CoinDrand48()*0.3);
          iType=0;
          break;
        }
      }
    }
    if (iType==-1&&random<0.8) {
      // trying addRow
      int j;
      for (j=0;j<3;j++) {
        int k;
        for (k=0;k<3;k++) {
          if (block[j+3*k])
            break; //already taken
        }
        if (k==3) {
          // can do but decide which one
          iBlock = j + 3*((int) (CoinDrand48()*0.3));
          iType=1;
          break;
        }
      }
    }
    if (iType==-1) {
      iType=2;
      int j;
      for (j=0;j<9;j++) {
        if (!block[j]) {
          iBlock=j;
          break;
        }
      }
    }
    random=CoinDrand48();
    assert (iBlock>=0&&iBlock<9);
    assert (iType>=0);
    assert (!block[iBlock]);
    block[iBlock]=static_cast<char>(1+iType);
    int jRow = iBlock/3;
    int jColumn = iBlock%3;
    bool doRow = (CoinDrand48()<0.5)&&!rowDone[jRow];
    bool doColumn = (CoinDrand48()<0.5&&!columnDone[jColumn]);
    if (!iType) {
      // addRow
      int * column = new int[lastColumn[jColumn+1]-lastColumn[jColumn]];
      double * element = new double[lastColumn[jColumn+1]-lastColumn[jColumn]];
      for (i=lastRow[jRow];i<lastRow[jRow+1];i++) {
        int n=0;
        CoinModelLink triple=baseModel.firstInRow(i);
        while (triple.column()>=0) {
          if (triple.column()>=lastColumn[jColumn]&&triple.column()<lastColumn[jColumn+1]) {
            column[n]=triple.column();
            element[n++]=triple.value();
          }
          triple=baseModel.next(triple);
        }
        assert (i>=model.numberRows());
        if (i>model.numberRows()) {
          assert (i==lastRow[jRow]);
	  std::cout << "need to fill in rows" << std::endl ;
          for (int k=0;k<jRow;k++) {
            int start = CoinMax(lastRow[k],model.numberRows());
            int end = lastRow[k+1];
            bool doBounds = rowDone[k]!=0;
            for (int j=start;j<end;j++) {
              if (doBounds)
                model.addRow(0,NULL,NULL,baseModel.rowLower(j),
                                baseModel.rowUpper(j),baseModel.rowName(j));
              else
                model.addRow(0,NULL,NULL);
            }
          }
        }
        if (doRow)
          model.addRow(n,column,element,baseModel.rowLower(i),
                       baseModel.rowUpper(i),baseModel.rowName(i));
        else
          model.addRow(n,column,element);
      } 
      if (doRow) {
        rowDone[jRow]=1;
      }
      delete [] column;
      delete [] element;
    } else if (iType==1) {
      // addColumn
      int * row = new int[lastRow[jRow+1]-lastRow[jRow]];
      double * element = new double[lastRow[jRow+1]-lastRow[jRow]];
      for (i=lastColumn[jColumn];i<lastColumn[jColumn+1];i++) {
        int n=0;
        CoinModelLink triple=baseModel.firstInColumn(i);
        while (triple.row()>=0) {
          if (triple.row()>=lastRow[jRow]&&triple.row()<lastRow[jRow+1]) {
            row[n]=triple.row();
            element[n++]=triple.value();
          }
          triple=baseModel.next(triple);
        }
        assert (i>=model.numberColumns());
        if (i>model.numberColumns()) {
          assert (i==lastColumn[jColumn]);
	  std::cout << "need to fill in columns" << std::endl ;
          for (int k=0;k<jColumn;k++) {
            int start = CoinMax(lastColumn[k],model.numberColumns());
            int end = lastColumn[k+1];
            bool doBounds = columnDone[k]!=0;
            for (int j=start;j<end;j++) {
              if (doBounds)
                model.addColumn(0,NULL,NULL,baseModel.columnLower(j),
                                baseModel.columnUpper(j),baseModel.objective(j),
                                baseModel.columnName(j),baseModel.isInteger(j));
              else
                model.addColumn(0,NULL,NULL);
            }
          }
        }
        if (doColumn)
          model.addColumn(n,row,element,baseModel.columnLower(i),
                       baseModel.columnUpper(i),baseModel.objective(i),
                          baseModel.columnName(i),baseModel.isInteger(i));
        else
          model.addColumn(n,row,element);
      } 
      if (doColumn) {
        columnDone[jColumn]=1;
      }
      delete [] row;
      delete [] element;
    } else {
      // addElement
      // Which way round ?
      if (CoinDrand48()<0.5) {
        // rim first
        if (doRow) {
          for (i=lastRow[jRow];i<lastRow[jRow+1];i++) {
            model.setRowLower(i,baseModel.getRowLower(i));
            model.setRowUpper(i,baseModel.getRowUpper(i));
            model.setRowName(i,baseModel.getRowName(i));
          }
          rowDone[jRow]=1;
        }
        if (doColumn) {
          for (i=lastColumn[jColumn];i<lastColumn[jColumn+1];i++) {
            model.setColumnLower(i,baseModel.getColumnLower(i));
            model.setColumnUpper(i,baseModel.getColumnUpper(i));
            model.setColumnName(i,baseModel.getColumnName(i));
            model.setColumnObjective(i,baseModel.getColumnObjective(i));
            model.setColumnIsInteger(i,baseModel.getColumnIsInteger(i));
          }
          columnDone[jColumn]=1;
        }
        if (CoinDrand48()<0.3) {
          // by row
          for (i=lastRow[jRow];i<lastRow[jRow+1];i++) {
            CoinModelLink triple=baseModel.firstInRow(i);
            while (triple.column()>=0) {
              int iColumn = triple.column();
              if (iColumn>=lastColumn[jColumn]&&iColumn<lastColumn[jColumn+1])
                model(i,triple.column(),triple.value());
              triple=baseModel.next(triple);
            }
          }
        } else if (CoinDrand48()<0.6) {
          // by column
          for (i=lastColumn[jColumn];i<lastColumn[jColumn+1];i++) {
            CoinModelLink triple=baseModel.firstInColumn(i);
            while (triple.row()>=0) {
              int iRow = triple.row();
              if (iRow>=lastRow[jRow]&&iRow<lastRow[jRow+1])
                model(triple.row(),i,triple.value());
              triple=baseModel.next(triple);
            }
          }
        } else {
          // scan through backwards
          const CoinModelTriple * elements = baseModel.elements();
          for (i=baseModel.numberElements()-1;i>=0;i--) {
            int iRow = (int) rowInTriple(elements[i]);
            int iColumn = elements[i].column;
            if (iRow>=lastRow[jRow]&&iRow<lastRow[jRow+1]&&
                iColumn>=lastColumn[jColumn]&&iColumn<lastColumn[jColumn+1])
              model(iRow,iColumn,elements[i].value);
          }
        }
      } else {
        // elements first
        if (CoinDrand48()<0.3) {
          // by row
          for (i=lastRow[jRow];i<lastRow[jRow+1];i++) {
            CoinModelLink triple=baseModel.firstInRow(i);
            while (triple.column()>=0) {
              int iColumn = triple.column();
              if (iColumn>=lastColumn[jColumn]&&iColumn<lastColumn[jColumn+1])
                model(i,triple.column(),triple.value());
              triple=baseModel.next(triple);
            }
          }
        } else if (CoinDrand48()<0.6) {
          // by column
          for (i=lastColumn[jColumn];i<lastColumn[jColumn+1];i++) {
            CoinModelLink triple=baseModel.firstInColumn(i);
            while (triple.row()>=0) {
              int iRow = triple.row();
              if (iRow>=lastRow[jRow]&&iRow<lastRow[jRow+1])
                model(triple.row(),i,triple.value());
              triple=baseModel.next(triple);
            }
          }
        } else {
          // scan through backwards
          const CoinModelTriple * elements = baseModel.elements();
          for (i=baseModel.numberElements()-1;i>=0;i--) {
            int iRow = (int) rowInTriple(elements[i]);
            int iColumn = elements[i].column;
            if (iRow>=lastRow[jRow]&&iRow<lastRow[jRow+1]&&
                iColumn>=lastColumn[jColumn]&&iColumn<lastColumn[jColumn+1])
              model(iRow,iColumn,elements[i].value);
          }
        }
        if (doRow) {
          for (i=lastRow[jRow];i<lastRow[jRow+1];i++) {
            model.setRowLower(i,baseModel.getRowLower(i));
            model.setRowUpper(i,baseModel.getRowUpper(i));
            model.setRowName(i,baseModel.getRowName(i));
          }
          rowDone[jRow]=1;
        }
        if (doColumn) {
          for (i=lastColumn[jColumn];i<lastColumn[jColumn+1];i++) {
            model.setColumnLower(i,baseModel.getColumnLower(i));
            model.setColumnUpper(i,baseModel.getColumnUpper(i));
            model.setColumnName(i,baseModel.getColumnName(i));
            model.setColumnObjective(i,baseModel.getColumnObjective(i));
            model.setColumnIsInteger(i,baseModel.getColumnIsInteger(i));
          }
          columnDone[jColumn]=1;
        }
      }
    }
    // Mess up be deleting some elements
    if (CoinDrand48()<0.3&&iPass>10) {
      double random2=CoinDrand48();
      double randomDelete = 0.2 + 0.5*random2; // fraction to delete
      //model.validateLinks();
      if (random2<0.2) {
        // delete some rows
        for (int j=lastRow[jRow];j<lastRow[jRow+1];j++) {
          //model.validateLinks();
          if (CoinDrand48()<randomDelete) {
            // save and delete
            double rowLower = model.rowLower(j);
            double rowUpper = model.rowUpper(j);
            char * rowName = CoinStrdup(model.rowName(j));
            CoinModelLink triple=model.firstInRow(j);
            while (triple.column()>=0) {
              int iColumn = triple.column();
              assert (j==triple.row());
              setRowInTriple(dTriple[numberElements],j);
              dTriple[numberElements].column=iColumn;
              dTriple[numberElements].value = triple.value();
              numberElements++;
              triple=model.next(triple);
            }
            model.deleteRow(j);
            if (rowDone[jRow]) {
              // put back rim
              model.setRowLower(j,rowLower);
              model.setRowUpper(j,rowUpper);
              model.setRowName(j,rowName);
            }
            free(rowName);
          }
        }
      } else if (random2<0.4) {
        // delete some columns
        for (int j=lastColumn[jColumn];j<lastColumn[jColumn+1];j++) {
          //model.validateLinks();
          if (CoinDrand48()<randomDelete) {
            // save and delete
            double columnLower = model.columnLower(j);
            double columnUpper = model.columnUpper(j);
            double objective = model.objective(j);
            bool integer = model.isInteger(j)!=0;
            char * columnName = CoinStrdup(model.columnName(j));
            CoinModelLink triple=model.firstInColumn(j);
            while (triple.column()>=0) {
              int iRow = triple.row();
              assert (j==triple.column());
              dTriple[numberElements].column = j;
              setRowInTriple(dTriple[numberElements],iRow);
              dTriple[numberElements].value = triple.value();
              numberElements++;
              triple=model.next(triple);
            }
            model.deleteColumn(j);
            if (columnDone[jColumn]) {
              // put back rim
              model.setColumnLower(j,columnLower);
              model.setColumnUpper(j,columnUpper);
              model.setObjective(j,objective);
              model.setIsInteger(j,integer);
              model.setColumnName(j,columnName);
            }
            free(columnName);
          }
        }
      } else {
        // delete some elements
        //model.validateLinks();
        const CoinModelTriple * elements = baseModel.elements();
        for (i=0;i<model.numberElements();i++) {
          int iRow = (int) rowInTriple(elements[i]);
          int iColumn = elements[i].column;
          if (iRow>=lastRow[jRow]&&iRow<lastRow[jRow+1]&&
              iColumn>=lastColumn[jColumn]&&iColumn<lastColumn[jColumn+1]) {
            if (CoinDrand48()<randomDelete) {
              dTriple[numberElements].column = iColumn;
              setRowInTriple(dTriple[numberElements],iRow);
              dTriple[numberElements].value = elements[i].value;
              int position = model.deleteElement(iRow,iColumn);
              if (position>=0)
                numberElements++;
            }
          }
        }
      }
    }
  }
  // Do rim if necessary
  for (int k=0;k<3;k++) {
    if (!rowDone[k]) {
      for (i=lastRow[k];i<lastRow[k+1];i++) {
        model.setRowLower(i,baseModel.getRowLower(i));
        model.setRowUpper(i,baseModel.getRowUpper(i));
        model.setRowName(i,baseModel.getRowName(i));
      }
    }
    if (!columnDone[k]) {
      for (i=lastColumn[k];i<lastColumn[k+1];i++) {
        model.setColumnLower(i,baseModel.getColumnLower(i));
        model.setColumnUpper(i,baseModel.getColumnUpper(i));
        model.setColumnName(i,baseModel.getColumnName(i));
        model.setColumnObjective(i,baseModel.getColumnObjective(i));
        model.setColumnIsInteger(i,baseModel.getColumnIsInteger(i));
      }
    }
  }
  // Put back any elements
  for (i=0;i<numberElements;i++) {
    model(rowInTriple(dTriple[i]),dTriple[i].column,dTriple[i].value);
  }
  delete [] dTriple;
  timeIt +=  CoinCpuTime()-time1;
  model.setLogLevel(1);
  assert (!model.differentModel(baseModel,false));
}
//--------------------------------------------------------------------------
// Test building a model
void
CoinModelUnitTest(const std::string & mpsDir,
                  const std::string & netlibDir, const std::string & testModel)
{
  
  // Get a model
  CoinMpsIO m;
  std::string fn = mpsDir+"exmip1";
  int numErr = m.readMps(fn.c_str(),"mps");
  assert( numErr== 0 );

  int numberRows = m.getNumRows();
  int numberColumns = m.getNumCols();

  // Build by row from scratch
  {
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
    CoinModel temp;
    for (i=0;i<numberRows;i++) {
      temp.addRow(rowLength[i],column+rowStart[i],
                  element+rowStart[i],rowLower[i],rowUpper[i],m.rowName(i));
    }
    // Now do column part
    for (i=0;i<numberColumns;i++) {
      temp.setColumnBounds(i,columnLower[i],columnUpper[i]);
      temp.setColumnObjective(i,objective[i]);
      if (m.isInteger(i))
        temp.setColumnIsInteger(i,true);;
    }
    // write out
    temp.writeMps("byRow.mps");
  }

  // Build by column from scratch and save
  CoinModel model;
  {
    CoinPackedMatrix matrixByColumn = * m.getMatrixByCol();
    const double * element = matrixByColumn.getElements();
    const int * row = matrixByColumn.getIndices();
    const CoinBigIndex * columnStart = matrixByColumn.getVectorStarts();
    const int * columnLength = matrixByColumn.getVectorLengths();
    const double * rowLower = m.getRowLower();
    const double * rowUpper = m.getRowUpper();
    const double * columnLower = m.getColLower();
    const double * columnUpper = m.getColUpper();
    const double * objective = m.getObjCoefficients();
    int i;
    for (i=0;i<numberColumns;i++) {
      model.addColumn(columnLength[i],row+columnStart[i],
                      element+columnStart[i],columnLower[i],columnUpper[i],
                      objective[i],m.columnName(i),m.isInteger(i));
    }
    // Now do row part
    for (i=0;i<numberRows;i++) {
      model.setRowBounds(i,rowLower[i],rowUpper[i]);
    }
    // write out
    model.writeMps("byColumn.mps");
  }

  // model was created by column - play around
  {
    CoinModel temp;
    int i;
    for (i=numberRows-1;i>=0;i--) 
      temp.setRowLower(i,model.getRowLower(i));
    for (i=0;i<numberColumns;i++) {
      temp.setColumnUpper(i,model.getColumnUpper(i));
      temp.setColumnName(i,model.getColumnName(i));
    }
    for (i=numberColumns-1;i>=0;i--) {
      temp.setColumnLower(i,model.getColumnLower(i));
      temp.setColumnObjective(i,model.getColumnObjective(i));
      temp.setColumnIsInteger(i,model.getColumnIsInteger(i));
    }
    for (i=0;i<numberRows;i++) {
      temp.setRowUpper(i,model.getRowUpper(i));
      temp.setRowName(i,model.getRowName(i));
    }
    // Now elements
    for (i=0;i<numberRows;i++) {
      CoinModelLink triple=model.firstInRow(i);
      while (triple.column()>=0) {
        temp(i,triple.column(),triple.value());
        triple=model.next(triple);
      }
    }
    // and by column
    for (i=numberColumns-1;i>=0;i--) {
      CoinModelLink triple=model.lastInColumn(i);
      while (triple.row()>=0) {
        assert (triple.value()==temp(triple.row(),i));
        temp(triple.row(),i,triple.value());
        triple=model.previous(triple);
      }
    }
    // check equal
    model.setLogLevel(1);
    assert (!model.differentModel(temp,false));
  }
  // Try creating model with strings
  {
    CoinModel temp;
    int i;
    for (i=numberRows-1;i>=0;i--) {
      double value = model.getRowLower(i);
      if (value==-1.0)
        temp.setRowLower(i,"minusOne");
      else if (value==1.0)
        temp.setRowLower(i,"sqrt(plusOne)");
      else if (value==4.0)
        temp.setRowLower(i,"abs(4*plusOne)");
      else
        temp.setRowLower(i,value);
    }
    for (i=0;i<numberColumns;i++) {
      double value;
      value = model.getColumnUpper(i);
      if (value==-1.0)
        temp.setColumnUpper(i,"minusOne");
      else if (value==1.0)
        temp.setColumnUpper(i,"plusOne");
      else
        temp.setColumnUpper(i,value);
      temp.setColumnName(i,model.getColumnName(i));
    }
    for (i=numberColumns-1;i>=0;i--) {
      temp.setColumnLower(i,model.getColumnLower(i));
      temp.setColumnObjective(i,model.getColumnObjective(i));
      temp.setColumnIsInteger(i,model.getColumnIsInteger(i));
    }
    for (i=0;i<numberRows;i++) {
      double value = model.getRowUpper(i);
      if (value==-1.0)
        temp.setRowUpper(i,"minusOne");
      else if (value==1.0)
        temp.setRowUpper(i,"plusOne");
      else
        temp.setRowUpper(i,value);
      temp.setRowName(i,model.getRowName(i));
    }
    // Now elements
    for (i=0;i<numberRows;i++) {
      CoinModelLink triple=model.firstInRow(i);
      while (triple.column()>=0) {
        double value = triple.value();
        if (value==-1.0)
          temp(i,triple.column(),"minusOne");
        else if (value==1.0)
          temp(i,triple.column(),"plusOne");
        else if (value==-2.0)
          temp(i,triple.column(),"minusOne-1.0");
        else if (value==2.0)
          temp(i,triple.column(),"plusOne+1.0+minusOne+(2.0-plusOne)");
        else
          temp(i,triple.column(),value);
        triple=model.next(triple);
      }
    }
    temp.associateElement("minusOne",-1.0);
    temp.associateElement("plusOne",1.0);
    temp.setProblemName("fromStrings");
    temp.writeMps("string.mps");
    
    // check equal
    model.setLogLevel(1);
    assert (!model.differentModel(temp,false));
  }
  // Test with various ways of generating
  {
    /*
      Get a model. Try first with netlibDir, fall back to mpsDir (sampleDir)
      if that fails.
    */
    CoinMpsIO m;
    std::string fn = netlibDir+testModel;
    double time1 = CoinCpuTime();
    int numErr = m.readMps(fn.c_str(),"");
    if (numErr != 0) {
      std::cout
	<< "Could not read " << testModel << " in " << netlibDir
	<< "; falling back to " << mpsDir << "." << std::endl ;
      fn = mpsDir+testModel ;
      numErr = m.readMps(fn.c_str(),"") ;
      if (numErr != 0) {
	std::cout << "Could not read " << testModel << "; skipping test." << std::endl ;
      }
    }
    if (numErr == 0) {
      std::cout
	<< "Time for readMps is "
	<< (CoinCpuTime()-time1) << " seconds." << std::endl ;
      int numberRows = m.getNumRows();
      int numberColumns = m.getNumCols();
      // Build model
      CoinModel model;
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
        model.addRow(rowLength[i],column+rowStart[i],
                     element+rowStart[i],rowLower[i],rowUpper[i],m.rowName(i));
      }
      // Now do column part
      for (i=0;i<numberColumns;i++) {
        model.setColumnBounds(i,columnLower[i],columnUpper[i]);
        model.setColumnObjective(i,objective[i]);
        model.setColumnName(i,m.columnName(i));
        if (m.isInteger(i))
          model.setColumnIsInteger(i,true);;
      }
      // Test
      CoinSeedRandom(11111);
      time1 = 0.0;
      int nPass=50;
      for (i=0;i<nPass;i++) {
        double random = CoinDrand48();
        int iSeed = (int) (random*1000000);
        //iSeed = 776151;
        CoinSeedRandom(iSeed);
	std::cout << "before pass " << i << " with seed of " << iSeed << std::endl ;
        buildRandom(model,CoinDrand48(),time1,i);
        model.validateLinks();
      }
      std::cout
	<< "Time for " << nPass << " CoinModel passes is "
	<< time1 << " seconds\n" << std::endl ;
    }
  }
}





