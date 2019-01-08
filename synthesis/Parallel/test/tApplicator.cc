#include <synthesis/Parallel/Applicator.h>
#include <synthesis/Parallel/Algorithm.h>
#include <synthesis/Parallel/PTransport.h>
#include <casacore/casa/Arrays/Vector.h>
#include <casacore/casa/BasicSL/Complex.h>
#include <casacore/casa/BasicSL/String.h>
#include <casacore/casa/iostream.h>

#include <casacore/casa/namespace.h>
extern casacore::Applicator casacore::applicator;

class TestAlgorithm : public Algorithm {
   public :
      TestAlgorithm() : myName(String("Test Algorithm")){}
     ~TestAlgorithm(){};

      void get();
      void put();
      String &name(){return myName;}
   private :
      Int      one;
      Float    two;
      Double   three;
      Complex  four;
      DComplex five;
      String   six;
      Bool     seven;
      
      Vector<Int>      aOne;
      Vector<Float>    aTwo;
      Vector<Double>   aThree;
      Vector<Complex>  aFour;
      Vector<DComplex> aFive;
      void             task();
      String           myName;
};

void TestAlgorithm::get(){

      cout << "In TestAlgorithm::get" << endl;
      casacore::applicator.get(one);
      cout << "got one " << one << endl;
      casacore::applicator.get(two);
      cout << "got two " << two << endl;
      casacore::applicator.get(three);
      cout << "got three " << three << endl;
      casacore::applicator.get(four);
      cout << "got four " << four << endl;
      casacore::applicator.get(five);
      cout << "got five " << five << endl;
      casacore::applicator.get(six);
      cout << "got six " << six << endl;
      casacore::applicator.get(seven);
      cout << "got seven " << seven << endl;

      casacore::applicator.get(aOne);
      cout << "got aOne " << endl;
      casacore::applicator.get(aTwo);
      cout << "got aTwo " << endl;
      casacore::applicator.get(aThree);
      cout << "got aThree " << endl;
      casacore::applicator.get(aFour);
      cout << "got aFour " << endl;
      casacore::applicator.get(aFive);
      cout << "got aFive " << endl;

      return;
}

void TestAlgorithm::put(){
      casacore::applicator.put(True);
      return;
}

void TestAlgorithm::task(){
      cout << "Do work now!" << endl;
      return;
}

//  OK the test program

int main(Int argc, Char *argv[]){

   TestAlgorithm testMe;
   casacore::applicator.defineAlgorithm(&testMe);
   casacore::applicator.init(argc, argv);
   if(casacore::applicator.isController()){
      Int rank(1);
      casacore::applicator.nextAvailProcess(testMe, rank);

      Int      one(1);
      Float    two(2.0f);
      Double   three(3.0);
      Complex  four(4.0,4.0);
      DComplex five(5.0,5.0);
      String   six("Six");
      Bool     seven(True);
      cout << "one " << one << endl;
      cout << "two " << two << endl;
      cout << "three " << three << endl;
      cout << "four " << four << endl;
      cout << "five " << five << endl;
      cout << "six " << six << endl;
      cout << "seven " << seven << endl;

      casacore::applicator.put(one);
      casacore::applicator.put(two);
      casacore::applicator.put(three);
      casacore::applicator.put(four);
      casacore::applicator.put(five);
      casacore::applicator.put(six);
      casacore::applicator.put(seven);
      
      Vector<Int>      aOne(3,one);
      Vector<Float>    aTwo(4,two);
      Vector<Double>   aThree(5,three);
      Vector<Complex>  aFour(6,four);
      Vector<DComplex> aFive(7,five);

      casacore::applicator.put(aOne);
      casacore::applicator.put(aTwo);
      casacore::applicator.put(aThree);
      casacore::applicator.put(aFour);
      casacore::applicator.put(aFive);

      casacore::applicator.apply(testMe);
      Bool status;
      casacore::applicator.get(status);
/*
      Bool allDone;
      applicator.nextProcessDone(testMe, allDone);
*/

      return 0;

   }
   return 0;
}
