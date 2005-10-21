#include "CNoise.h"

using namespace std;
void CNoise::getNuG(CMatrix& g, CMatrix& nu, const int index) const
{
  assert(g.dimensionsMatch(nu));
  assert(g.getRows()==nData);
  assert(g.getCols()==nProcesses);
  assert(index>=0);
  assert(index<nData);
  double nuval=0.0;
  double gval=0.0;
  
  for(int j=0; j<g.getCols(); j++)
    {
      getGradInputs(gval, nuval, index, j);
      nuval = gval*gval - 2*nuval;
      if(nuval<0)
	{
	  if(isLogConcave())
	    {
	      throw ndlexceptions::Error("Log concave noise model has value of nu < 0.");
	    }
	  else 
	    {
	      nuval=ndlutil::SMALLVAL;
	    }
	}
      if(isnan(nuval)) 
	throw ndlexceptions::Error("Nu is NaN");
      if(abs(nuval)<ndlutil::SMALLVAL)
	nuval=ndlutil::EPS;
      nu.setVal(nuval, index, j);
      
      g.setVal(gval, index, j);
    }
  
  
}

void CNoise::updateSites(CMatrix& m, CMatrix& beta, const int actIndex, 
				 const CMatrix& g, const CMatrix& nu, 
				 const int index) const
{
  assert(index>=0);
  assert(index<nData);
  assert(m.dimensionsMatch(beta));
  assert(actIndex>=0);
  assert(actIndex<m.getRows());
  assert(g.dimensionsMatch(nu));
  assert(m.getCols()==g.getCols());
  double nuVal=0.0;
  double gVal=0.0;
  for(int j=0; j<m.getCols(); j++)
    {
      nuVal = nu.getVal(index, j);
      beta.setVal(nuVal/(1-nuVal*getVarSigma(index, j)), actIndex, j);
      m.setVal(getMu(index, j) + g.getVal(index, j)/nuVal, actIndex, j);
    }
}
#ifdef _NDLMATLAB
mxArray* CNoise::toMxArray() const
{
  int dims[1];
  dims[0] = 1;
  const char *fieldNames[] = {"type", "transforms", "numProcess", "spherical", "nParams", "missing", "logconcave"};

  mxArray* matlabArray = mxCreateStructArray(1, dims, 7, fieldNames);
    
  // type field.
  const char *typeName[1];
  string ty=getType();
  typeName[0] = ty.c_str();
  mxSetField(matlabArray, 0, "type", 
	     mxCreateCharMatrixFromStrings(1, typeName));
  
  // transforms field.
  mxSetField(matlabArray, 0, "transforms", transformsToMxArray());

  // inputDimension field.
  mxSetField(matlabArray, 0, "numProcess", convertMxArray((double)nProcesses));

  // nParams field.
  mxSetField(matlabArray, 0, "nParams", convertMxArray((double)nParams));
    
  // spherical field
  mxSetField(matlabArray, 0, "spherical", convertMxArray((double)spherical));

  // missing field
  mxSetField(matlabArray, 0, "missing", convertMxArray((double)missing));
  
  // logConcave field
  mxSetField(matlabArray, 0, "logconcave", convertMxArray((double)logConcave));
// priors field.
  /// if priors exist need to add the field.

  // Noise specific code.
  addParamToMxArray(matlabArray);
  return matlabArray;

}
void CNoise::fromMxArray(const mxArray* matlabArray) 
{
  string mxType = mxArrayExtractStringField(matlabArray, "type");
  if(mxType!=type)
    cerr << "Error mismatch between saved type, " << mxType << ", and Class type, " << type << "." << endl;
  
  mxArray* transformArray = mxArrayExtractMxArrayField(matlabArray, "transforms");
  if(transformArray!=NULL)
    transformsFromMxArray(transformArray);
  nProcesses = mxArrayExtractIntField(matlabArray, "numProcess");
  spherical = mxArrayExtractBoolField(matlabArray, "spherical");
  logConcave = mxArrayExtractBoolField(matlabArray, "logconcave");
  missing = mxArrayExtractBoolField(matlabArray, "missing");
  initStoreage();
  // TODO priors ... need to deal with priors
  extractParamFromMxArray(matlabArray);
}
  
  
void CNoise::addParamToMxArray(mxArray* matlabArray) const
{
  mxAddField(matlabArray, "nParams");
  mxSetField(matlabArray, 0, "nParams", convertMxArray((double)nParams));
  string pName;
  for(int i=0; i<nParams; i++)
    {
      pName = getParamName(i);
      mxAddField(matlabArray, pName.c_str());      
      mxSetField(matlabArray, 0, pName.c_str(), convertMxArray(getParam(i))); 
    } 
}

void CNoise::extractParamFromMxArray(const mxArray* matlabArray) 
{
  nParams = mxArrayExtractIntField(matlabArray, "nParams");
  string pName;
  for(int i=0; i<nParams; i++)
    {
      pName=getParamName(i);
      setParam(mxArrayExtractDoubleField(matlabArray, pName), i);
    }

}
void CGaussianNoise::addParamToMxArray(mxArray* matlabArray) const
{
  mxAddField(matlabArray, "nParams");
  mxSetField(matlabArray, 0, "nParams", convertMxArray((double)getNumParams()));
  mxAddField(matlabArray, "bias");
  mxSetField(matlabArray, 0, "bias", bias.toMxArray());
  mxAddField(matlabArray, "sigma2");
  mxSetField(matlabArray, 0, "sigma2", convertMxArray(sigma2));
}

void CGaussianNoise::extractParamFromMxArray(const mxArray* matlabArray) 
{
  setNumParams(mxArrayExtractIntField(matlabArray, "nParams"));
  mxArray* biasField = mxArrayExtractMxArrayField(matlabArray, "bias");
  bias.fromMxArray(biasField);
  sigma2 = mxArrayExtractDoubleField(matlabArray, "sigma2");

}
void CScaleNoise::addParamToMxArray(mxArray* matlabArray) const
{
  mxAddField(matlabArray, "nParams");
  mxSetField(matlabArray, 0, "nParams", convertMxArray((double)getNumParams()));
  mxAddField(matlabArray, "bias");
  mxSetField(matlabArray, 0, "bias", bias.toMxArray());
  mxAddField(matlabArray, "scale");
  mxSetField(matlabArray, 0, "scale", scale.toMxArray());
}

void CScaleNoise::extractParamFromMxArray(const mxArray* matlabArray) 
{
  setNumParams(mxArrayExtractIntField(matlabArray, "nParams"));
  mxArray* biasField = mxArrayExtractMxArrayField(matlabArray, "bias");
  bias.fromMxArray(biasField);
  mxArray* scaleField = mxArrayExtractMxArrayField(matlabArray, "scale");
  scale.fromMxArray(scaleField);

}

void CProbitNoise::addParamToMxArray(mxArray* matlabArray) const
{
  mxAddField(matlabArray, "nParams");
  mxSetField(matlabArray, 0, "nParams", convertMxArray((double)getNumParams()));
  mxAddField(matlabArray, "bias");
  mxSetField(matlabArray, 0, "bias", bias.toMxArray());
  mxAddField(matlabArray, "sigma2");
  mxSetField(matlabArray, 0, "sigma2", convertMxArray(sigma2));
}

void CProbitNoise::extractParamFromMxArray(const mxArray* matlabArray) 
{
  setNumParams(mxArrayExtractIntField(matlabArray, "nParams"));
  mxArray* biasField = mxArrayExtractMxArrayField(matlabArray, "bias");
  bias.fromMxArray(biasField);
  sigma2 = mxArrayExtractDoubleField(matlabArray, "sigma2");

}
void CNcnmNoise::addParamToMxArray(mxArray* matlabArray) const
{
  mxAddField(matlabArray, "nParams");
  mxSetField(matlabArray, 0, "nParams", convertMxArray((double)getNumParams()));
  mxAddField(matlabArray, "bias");
  mxSetField(matlabArray, 0, "bias", bias.toMxArray());
  mxAddField(matlabArray, "sigma2");
  mxSetField(matlabArray, 0, "sigma2", convertMxArray(sigma2));
  mxAddField(matlabArray, "width");
  mxSetField(matlabArray, 0, "width", convertMxArray(width));
  mxAddField(matlabArray, "gamman");
  mxSetField(matlabArray, 0, "gamman", convertMxArray(gamman));
  mxAddField(matlabArray, "gammap");
  mxSetField(matlabArray, 0, "gammap", convertMxArray(gammap));
  mxAddField(matlabArray, "gammaSplit");
  mxSetField(matlabArray, 0, "gammaSplit", convertMxArray(isSplitGamma()));
}

void CNcnmNoise::extractParamFromMxArray(const mxArray* matlabArray) 
{
  setNumParams(mxArrayExtractIntField(matlabArray, "nParams"));
  mxArray* biasField = mxArrayExtractMxArrayField(matlabArray, "bias");
  bias.fromMxArray(biasField);
  sigma2 = mxArrayExtractDoubleField(matlabArray, "sigma2");
  width = mxArrayExtractDoubleField(matlabArray, "width");
  gamman = mxArrayExtractDoubleField(matlabArray, "gamman");
  gammap = mxArrayExtractDoubleField(matlabArray, "gammap");
  int temp = mxArrayExtractIntField(matlabArray, "gammaSplit");
  if(temp)
    splitGamma = true;
  else
    splitGamma = false;

}
void COrderedNoise::addParamToMxArray(mxArray* matlabArray) const
{
  mxAddField(matlabArray, "nParams");
  mxSetField(matlabArray, 0, "nParams", convertMxArray((double)getNumParams()));
  mxAddField(matlabArray, "bias");
  mxSetField(matlabArray, 0, "bias", bias.toMxArray());
  mxAddField(matlabArray, "variance");
  mxSetField(matlabArray, 0, "variance", convertMxArray(sigma2));
  mxAddField(matlabArray, "widths");
  mxSetField(matlabArray, 0, "widths", widths.toMxArray());
  mxAddField(matlabArray, "C");
  mxSetField(matlabArray, 0, "C", convertMxArray((double)getNumCategories()));
}

void COrderedNoise::extractParamFromMxArray(const mxArray* matlabArray) 
{
  setNumParams(mxArrayExtractIntField(matlabArray, "nParams"));
  mxArray* biasField = mxArrayExtractMxArrayField(matlabArray, "bias");
  bias.fromMxArray(biasField);
  sigma2 = mxArrayExtractDoubleField(matlabArray, "variance");
  mxArray* widthsField = mxArrayExtractMxArrayField(matlabArray, "widths");
  widths.fromMxArray(widthsField);
  setNumCategories(mxArrayExtractIntField(matlabArray, "C"));
}


#endif
bool CNoise::equals(const CNoise& noise, const double tol) const
{
  if(getType()!=noise.getType())
    return false;
  if(getNumParams()!=noise.getNumParams())
    return false;
  CMatrix params(1, getNumParams());
  getParams(params);
  CMatrix noiseParams(1, getNumParams());
  noise.getParams(noiseParams);
  if(!params.equals(noiseParams, tol))
    return false;
  return true;
}
void CNoise::writeParamsToStream(ostream& out) const
{
  out << "numParams=" << getNumParams() << endl;
  out << "numProcesses=" << getNumProcesses() << endl;
  for(int i=0; i<getNumParams()-1; i++)
    out << getParam(i) << " ";
  out << getParam(getNumParams()-1) << endl;
}
void CNoise::readParamsFromStream(istream& in)
{
  string line;
  vector<string> tokens;
  getline(in, line);
  ndlstrutil::tokenise(tokens, line, "=");
  if(tokens.size()>2 || tokens[0]!="numParams")
    throw ndlexceptions::FileFormatError();
  int numPar=atol(tokens[1].c_str());
  tokens.clear();
  getline(in, line);
  ndlstrutil::tokenise(tokens, line, "=");
  if(tokens.size()>2 || tokens[0]!="numProcesses")
    throw ndlexceptions::FileFormatError();
  int numProc=atol(tokens[1].c_str());
  CMatrix par(1, numPar);
  tokens.clear();
  getline(in, line);
  ndlstrutil::tokenise(tokens, line, " ");
  for(int i=0; i<numPar; i++)
    par.setVal(atof(tokens[i].c_str()), i);
  setNumProcesses(numProc);
  if(numPar==getNumParams())
    setParams(par);
  else
    throw ndlexceptions::FileFormatError();
}

CGaussianNoise::~CGaussianNoise()
{
}
void CGaussianNoise::initStoreage()
{
  setNumParams(getNumProcesses()+1);
  mu.resize(getNumData(), getNumProcesses());
  varSigma.resize(getNumData(), getNumProcesses());
  bias.resize(1, getNumProcesses());
  clearTransforms();
  // transform sigma2 (the last parameter).
  addTransform(new CNegLogLogitTransform(), getNumParams()-1);
  sigma2 = 1e-6;
  setLogConcave(true);
  setSpherical(true);
  setMissing(false);

}
void CGaussianNoise::initNames()
{
  setType("gaussian");
  setNoiseName("Gaussian");
  for(int j=0; j<getNumProcesses(); j++)
    setParamName("bias" + j, j);
  setParamName("sigma2", getNumProcesses());

}
void CGaussianNoise::initParams()
{
  bias.deepCopy(meanCol(y));
}
void CGaussianNoise::initVals()
{
  mu.zeros();
  varSigma.zeros();
}
ostream& CGaussianNoise::display(ostream& os)
{
  double b = 0.0;
  os << "Gaussian Noise: " << endl; 
  for(int j=0; j<bias.getCols(); j++)
    {
      b = bias.getVal(j);
      os << "Bias on process " << j << ": " << b << endl; 
    }
  os << "Variance: " << sigma2 << endl;
  return os;
}
void CGaussianNoise::setParam(double val, int index)
{
  assert(index>=0);
  assert(index<getNumParams());
  if(index<getNumProcesses())
    bias.setVal(val, index);
  else
    sigma2=val;
}  
void CGaussianNoise::setParams(const CMatrix& params)
{
  assert(getNumParams()==getNumProcesses()+1);
  assert(params.getCols()==getNumParams());
  assert(params.getRows()==1);
  assert(getNumProcesses()==bias.getCols());
  for(int j=0; j<bias.getCols(); j++)
    {
      bias.setVal(params.getVal(j), j);
    }
  sigma2 = params.getVal(getNumParams()-1);
}
double CGaussianNoise::getParam(const int index) const
{
  assert(index>=0);
  assert(index<getNumParams());
  if(index<getNumProcesses())
    return bias.getVal(index);
  else
    return sigma2;

}
void CGaussianNoise::getParams(CMatrix& params) const
{
  assert(getNumParams()==getNumProcesses()+1);
  assert(params.getCols()==getNumParams());
  assert(params.getRows()==1);
  assert(getNumProcesses()==bias.getCols());
  for(int j=0; j<getNumProcesses(); j++)
    params.setVal(bias.getVal(j), j);
  params.setVal(sigma2, getNumParams()-1);
}
 
void CGaussianNoise::getGradParams(CMatrix& g) const
{
  assert(g.getCols()==getNumParams());
  assert(g.getRows()==1);
  double nu=0.0;
  double u=0.0;
  double gsigma2=0.0;
  double b=0.0;
  for(int j=0; j<getNumProcesses(); j++)
    {
      double gbias = 0.0;
      b = bias.getVal(j);
      for(int i=0; i<getNumData(); i++)
	{
	  nu = 1/(getVarSigma(i, j)+sigma2);
	  u=getTarget(i, j) - getMu(i, j)-b;
	  u*=nu;
	  gbias+=u;
	  gsigma2+=nu-u*u;
	}
      g.setVal(gbias, 0, j);
    }
  g.setVal(-0.5*gsigma2, 0, getNumParams()-1);
}

void CGaussianNoise::getGradInputs(double& gmu, double& gvs, const int i, const int j) const
{
  gmu = -bias.getVal(j);
  gvs = 1/(sigma2+getVarSigma(i, j));
  gmu += getTarget(i, j)-getMu(i, j);
  gmu *= gvs;
  gvs = 0.5*(gmu*gmu - gvs);
}
  
void CGaussianNoise::getNuG(CMatrix& g, CMatrix& nu, const int index) const
{
  double nuval=0.0;
  double gval=0.0;
  for(int j=0; j<getNumProcesses(); j++)
    {
      nuval=1./(sigma2+getVarSigma(index, j));
      if(isnan(nuval))
	{
	  cout << "Sigma2 " << sigma2 << endl;
	  cout << "varSigma " << getVarSigma(index, j) << endl;
	}
      assert(!isnan(nuval));
      nu.setVal(nuval, index, j);
      gval=getTarget(index, j)-getMu(index, j)-bias.getVal(j);
      g.setVal(gval*nuval, index, j);
    }
}
void CGaussianNoise::updateSites(CMatrix& m, CMatrix& beta, const int actIndex, 
				 const CMatrix& g, const CMatrix& nu, 
				 const int index) const
{
  for(int j=0; j<getNumProcesses(); j++)
    {
      beta.setVal(1/sigma2, actIndex, j);
      m.setVal(getTarget(index, j)-bias.getVal(j), actIndex, j);
    }
}
void CGaussianNoise::test(const CMatrix& muout, const CMatrix& varSigmaOut, const CMatrix& yTest) const
{
  assert(yTest.dimensionsMatch(muout));
  assert(muout.dimensionsMatch(varSigmaOut));
  assert(yTest.getCols()==getNumProcesses());
  CMatrix yPred(yTest.getRows(), yTest.getCols());
  out(yPred, muout, varSigmaOut);
  for(int i=0; i<getNumProcesses(); i++)
    cout << "Mean Squared Error on output " << i+1 << ": " << yPred.dist2Col(i, yTest, i)/(double)yTest.getRows() << endl;
}

void CGaussianNoise::out(CMatrix& yPred, const CMatrix& muTest, const CMatrix& varSigmaTest) const
{
  assert(yPred.dimensionsMatch(muTest));
  assert(muTest.dimensionsMatch(varSigmaTest));
  yPred.deepCopy(muTest);
  for(int j=0; j<getNumProcesses(); j++)
    yPred.addCol(j,bias.getVal(j));
}
void CGaussianNoise::out(CMatrix& yPred, CMatrix& errorBars, const CMatrix& muTest, const CMatrix& varSigmaTest) const
{
  assert(yPred.dimensionsMatch(errorBars));
  out(yPred, muTest, varSigmaTest);
  for(int i=0; i<errorBars.getRows(); i++)
    for(int j=0; j<errorBars.getCols(); j++)
      errorBars.setVal(sqrt(varSigmaTest.getVal(i, j) + sigma2), i, j);
}
void CGaussianNoise::likelihoods(CMatrix& L, const CMatrix& muTest, const CMatrix& varSigmaTest, 
				   const CMatrix& yTest) const
{
  assert(yTest.getCols()==getNumProcesses());
  assert(L.dimensionsMatch(muTest));
  assert(yTest.dimensionsMatch(muTest));
  assert(muTest.dimensionsMatch(varSigmaTest));
  double arg=0.0;
  double var=0.0;
  for(int i=0; i<muTest.getRows(); i++)
    {
      for(int j=0; j<muTest.getCols(); j++)
	{
	  arg = yTest.getVal(i, j) - muTest.getVal(i, j) - bias.getVal(j);
	  arg *= arg;
	  var = varSigmaTest.getVal(i, j) + sigma2;
	  arg = 1/sqrt(2*M_PI*var)*exp(-.5*arg*arg/var);
	  L.setVal(arg, i, j);
	}
    }
}

double CGaussianNoise::logLikelihood(const CMatrix& muTest, const CMatrix& varSigmaTest, 
				     const CMatrix& yTest) const
{
  assert(yTest.getCols()==getNumProcesses());
  assert(yTest.dimensionsMatch(muTest));
  assert(yTest.dimensionsMatch(varSigmaTest));
  double arg=0.0;
  double var=0.0;
  double L=0.0;
  for(int i=0; i<muTest.getRows(); i++)
    {
      for(int j=0; j<muTest.getCols(); j++)
	{
	  arg = yTest.getVal(i, j) - muTest.getVal(i, j) - bias.getVal(j);
	  arg *= arg;
	  var = varSigmaTest.getVal(i, j) + sigma2;
	  arg = arg/var;
	  L += log(var)+arg;
	}
    }  
  L += muTest.getRows()*muTest.getCols()*log(2*M_PI);
  L *= -0.5;
  return L;
}


CScaleNoise::~CScaleNoise()
{
}
void CScaleNoise::initStoreage()
{
  setNumParams(2*getNumProcesses());
  mu.resize(getNumData(), getNumProcesses());
  varSigma.resize(getNumData(), getNumProcesses());
  bias.resize(1, getNumProcesses());
  scale.resize(1, getNumProcesses());
  clearTransforms();
  sigma2=1e-6;
  setLogConcave(true);
  setSpherical(true);
  setMissing(false);

}
void CScaleNoise::initVals()
{
  mu.zeros();
  varSigma.zeros();
  bias.zeros();
  scale.ones();
}
void CScaleNoise::initNames()
{
  setType("scale");
  setNoiseName("Scaled Gaussian");
  for(int j=0; j<getNumProcesses(); j++)
    {
      setParamName("bias" + j, j);
      setParamName("scale" + j, getNumProcesses()+j);
    }
  
}
void CScaleNoise::initParams()
{
  scale.deepCopy(varCol(y));
  for(int j=0; j<getNumProcesses(); j++)
    {
      scale.setVal(sqrt(scale.getVal(j)), j);
      if(scale.getVal(j)<ndlutil::EPS)
	scale.setVal(ndlutil::EPS, j);
    }
  // TODO: need to check for missing values.
  bias.deepCopy(meanCol(y));
}
ostream& CScaleNoise::display(ostream& os)
{
  double b = 0.0;
  double s = 0.0;
  os << "Scale Noise: " << endl; 
  for(int j=0; j<bias.getCols(); j++)
    {
      b = bias.getVal(j);
      os << "Bias on process " << j << ": " << b << endl; 
      s = scale.getVal(j);
      os << "Scale on process " << j << ": " << s << endl; 
    }
  return os;
}
void CScaleNoise::setParam(double val, int index)
{
  assert(index>=0);
  assert(index<2*getNumParams()-1);
  if(index<getNumProcesses())
    bias.setVal(val, index);
  else if(index<2*getNumProcesses())
    scale.setVal(val, index-getNumProcesses());
}  
void CScaleNoise::setParams(const CMatrix& params)
{
  assert(getNumParams()==2*getNumProcesses());
  assert(params.getCols()==getNumParams());
  assert(params.getRows()==1);
  assert(getNumProcesses()==bias.getCols());
  for(int j=0; j<bias.getCols(); j++)
    {
      bias.setVal(params.getVal(j), j);
    }
  for(int j=0; j<scale.getCols(); j++)
    {
      scale.setVal(params.getVal(j+bias.getCols()), j);
    }
}
double CScaleNoise::getParam(const int index) const
{
  assert(index>=0);
  assert(index<2*getNumParams());
  if(index<getNumProcesses())
    return bias.getVal(index);
  else
    return scale.getVal(index-getNumProcesses());

}
void CScaleNoise::getParams(CMatrix& params) const
{
  assert(getNumParams()==2*getNumProcesses());
  assert(params.getCols()==getNumParams());
  assert(params.getRows()==1);
  assert(getNumProcesses()==bias.getCols());
  for(int j=0; j<getNumProcesses(); j++)
    params.setVal(bias.getVal(j), j);
  for(int j=0; j<getNumProcesses(); j++)
    params.setVal(scale.getVal(j), j+getNumProcesses());
}
 
void CScaleNoise::getGradParams(CMatrix& g) const
{
  throw ndlexceptions::Error("ScaleNoise doesn't have gradients implemented.");
  /*~
    assert(g.getCols()==getNumParams());
  assert(g.getRows()==1);
  double nu=0.0;
  double u=0.0;
  double gsigma2=0.0;
  double b=0.0;
  for(int j=0; j<getNumProcesses(); j++)
    {
      double gbias = 0.0;
      b = bias.getVal(j);
      s = scale.getVal(j);
      for(int i=0; i<getNumData(); i++)
	{
	  nu = 1/(getVarSigma(i, j)+sigma2);
	  u=getTarget(i, j) - getMu(i, j)-b;
	  u*=nu;
	  gbias+=u;
	  gsigma2+=nu-u*u;
	}
      g.setVal(gbias, 0, j);
      g.setVal(gscale, 0, j+getNumProcesses());
    }
    ~*/
}

void CScaleNoise::getGradInputs(double& gmu, double& gvs, const int i, const int j) const
{
  throw ndlexceptions::Error("ScaleNoise doesn't have gradients implemented.");
  /*~
  gmu = -bias.getVal(j);
  gvs = 1/(sigma2+getVarSigma(i, j));
  gmu += getTarget(i, j)-getMu(i, j);
  gmu *= gvs;
  gvs = 0.5*(gmu*gmu - gvs);
  ~*/
}
  
void CScaleNoise::getNuG(CMatrix& g, CMatrix& nu, const int index) const
{
  throw ndlexceptions::Error("ScaleNoise doesn't have gradients implemented.");
  /*~
  double nuval=0.0;
  double gval=0.0;
  for(int j=0; j<getNumProcesses(); j++)
    {
      nuval=1./(sigma2+getVarSigma(index, j));
      if(isnan(nuval))
	{
	  cout << "Sigma2 " << sigma2 << endl;
	  cout << "varSigma " << getVarSigma(index, j) << endl;
	}
      assert(!isnan(nuval));
      nu.setVal(nuval, index, j);
      gval=getTarget(index, j)-getMu(index, j)-bias.getVal(j);
      g.setVal(gval*nuval, index, j);
    }
    ~*/
}
void CScaleNoise::updateSites(CMatrix& m, CMatrix& beta, const int actIndex, 
				 const CMatrix& g, const CMatrix& nu, 
				 const int index) const
{
  for(int j=0; j<getNumProcesses(); j++)
    {
      beta.setVal(1/sigma2, actIndex, j);
      m.setVal((getTarget(index, j)-bias.getVal(j))/scale.getVal(j), actIndex, j);
    }
}
void CScaleNoise::test(const CMatrix& muout, const CMatrix& varSigmaOut, const CMatrix& yTest) const
{
  throw ndlexceptions::Error("ScaleNoise doesn't have site updates implemented.");
  /*~
  assert(yTest.dimensionsMatch(muout));
  assert(muout.dimensionsMatch(varSigmaOut));
  assert(yTest.getCols()==getNumProcesses());
  CMatrix yPred(yTest.getRows(), yTest.getCols());
  out(yPred, muout, varSigmaOut);
  for(int i=0; i<getNumProcesses(); i++)
    cout << "Mean Squared Error on output " << i+1 << ": " << yPred.dist2Col(i, yTest, i)/(double)yTest.getRows() << endl;
    ~*/
}

void CScaleNoise::out(CMatrix& yPred, const CMatrix& muTest, const CMatrix& varSigmaTest) const
{
  assert(yPred.dimensionsMatch(muTest));
  assert(muTest.dimensionsMatch(varSigmaTest));
  yPred.deepCopy(muTest);
  for(int j=0; j<getNumProcesses(); j++)
    {
      yPred.scaleCol(j, scale.getVal(j));
      yPred.addCol(j, bias.getVal(j));
    }
}
void CScaleNoise::out(CMatrix& yPred, CMatrix& errorBars, const CMatrix& muTest, const CMatrix& varSigmaTest) const
{
  assert(yPred.dimensionsMatch(errorBars));
  out(yPred, muTest, varSigmaTest);
  for(int i=0; i<errorBars.getRows(); i++)
    for(int j=0; j<errorBars.getCols(); j++)
      errorBars.setVal(sqrt(varSigmaTest.getVal(i, j) + sigma2)*scale.getVal(j), i, j);
}
void CScaleNoise::likelihoods(CMatrix& L, const CMatrix& muTest, const CMatrix& varSigmaTest, 
				   const CMatrix& yTest) const
{
  throw ndlexceptions::Error("ScaleNoise doesn't have site likelihoods implemented.");
  /*~
  assert(yTest.getCols()==getNumProcesses());
  assert(L.dimensionsMatch(muTest));
  assert(yTest.dimensionsMatch(muTest));
  assert(muTest.dimensionsMatch(varSigmaTest));
  double arg=0.0;
  double var=0.0;
  for(int i=0; i<muTest.getRows(); i++)
    {
      for(int j=0; j<muTest.getCols(); j++)
	{
	  arg = yTest.getVal(i, j) - muTest.getVal(i, j) - bias.getVal(j);
	  arg *= arg;
	  var = varSigmaTest.getVal(i, j) + sigma2;
	  arg = 1/sqrt(2*M_PI*var)*exp(-.5*arg*arg/var);
	  L.setVal(arg, i, j);
	}
    }
  ~*/
}

double CScaleNoise::logLikelihood(const CMatrix& muTest, const CMatrix& varSigmaTest, 
				     const CMatrix& yTest) const
{
  throw ndlexceptions::Error("ScaleNoise doesn't have site loglikelihoods implemented.");
  /*~
  assert(yTest.getCols()==getNumProcesses());
  assert(yTest.dimensionsMatch(muTest));
  assert(yTest.dimensionsMatch(varSigmaTest));
  double arg=0.0;
  double var=0.0;
  double L=0.0;
  for(int i=0; i<muTest.getRows(); i++)
    {
      for(int j=0; j<muTest.getCols(); j++)
	{
	  arg = yTest.getVal(i, j) - muTest.getVal(i, j) - bias.getVal(j);
	  arg *= arg;
	  var = varSigmaTest.getVal(i, j) + sigma2;
	  arg = arg/var;
	  L += log(var)+arg;
	}
    }  
  L += muTest.getRows()*muTest.getCols()*log(2*M_PI);
  L *= -0.5;
  return L;
  ~*/
}





CProbitNoise::~CProbitNoise()
{
}
void CProbitNoise::initStoreage()
{
  setNumParams(getNumProcesses());
  mu.resize(getNumData(), getNumProcesses());
  varSigma.resize(getNumData(), getNumProcesses());
  bias.resize(1, getNumProcesses());
  clearTransforms();
  sigma2 = 1e-6;
  setLogConcave(true);
  setSpherical(false);
  setMissing(false);

}
void CProbitNoise::initVals()
{
  mu.zeros();
  varSigma.zeros();
  bias.zeros();

}
void CProbitNoise::initNames()
{
  setType("probit");
  setNoiseName("Probit");
  for(int j=0; j<getNumProcesses(); j++)
    setParamName("bias" + j, j);

 
}
void CProbitNoise::initParams()
{
  for(int j=0; j<getNumProcesses(); j++)
    {
      double nClass1=0.0;
      for(int i=0; i<getNumData(); i++)
	{
	  if(y.getVal(i, j)==1)
	    nClass1++;
	}
      bias.setVal(ndlutil::invCumGaussian(nClass1/(double)getNumData()), j);
    }
}

ostream& CProbitNoise::display(ostream& os)
{
  double b = 0.0;
  os << "Probit noise: " << endl;
  for(int j=0; j<bias.getCols(); j++)
    {
      b = bias.getVal(j);
      os << "Bias on process " << j << ": " << b << endl; 
    }
  return os;
}
void CProbitNoise::setParam(double val, int index)
{
  assert(index>=0);
  assert(index<getNumParams());
  bias.setVal(val, index);
}  
void CProbitNoise::setParams(const CMatrix& params)
{
  assert(getNumParams()==getNumProcesses());
  assert(params.getCols()==getNumParams());
  assert(params.getRows()==1);
  assert(getNumProcesses()==bias.getCols());
  for(int j=0; j<bias.getCols(); j++)
    {
      bias.setVal(params.getVal(j), j);
    }
}
double CProbitNoise::getParam(const int index) const
{
  assert(index>=0);
  assert(index<getNumParams());
  if(index<getNumProcesses())
    return bias.getVal(index);
  return 0;
}
void CProbitNoise::getParams(CMatrix& params) const
{
  assert(getNumParams()==getNumProcesses());
  assert(params.getCols()==getNumParams());
  assert(params.getRows()==1);
  assert(getNumProcesses()==bias.getCols());
  for(int j=0; j<getNumProcesses(); j++)
    params.setVal(bias.getVal(j), j);
}
 
void CProbitNoise::getGradParams(CMatrix& g) const
{
  assert(g.getCols()==getNumParams());
  assert(g.getRows()==1);
  double c=0.0;
  double u=0.0;
  double b=0.0;
  for(int j=0; j<getNumProcesses(); j++)
    {
      double gbias = 0.0;
      b = bias.getVal(j);
      for(int i=0; i<getNumData(); i++)
	{
	  c = getTarget(i, j)/sqrt(getVarSigma(i, j)+sigma2);
	  u=c*(getMu(i, j)+b);
	  u=ndlutil::gradLnCumGaussian(u);
	  gbias+=u*c;
	}
      g.setVal(gbias, 0, j);
    }
}
void CProbitNoise::getGradInputs(double& gmu, double& gvs, const int i, const int j) const
{
  double b = bias.getVal(j);
  double c = getTarget(i, j)/sqrt(sigma2+getVarSigma(i, j));
  double u = c*(getMu(i, j) + b);
  gmu = ndlutil::gradLnCumGaussian(u)*c;
  gvs = -0.5*c*u*gmu;
}

void CProbitNoise::test(const CMatrix& muout, const CMatrix& varSigmaOut, const CMatrix& yTest) const
{
  assert(yTest.dimensionsMatch(muout));
  assert(muout.dimensionsMatch(varSigmaOut));
  assert(yTest.getCols()==getNumProcesses());
  CMatrix yPred(yTest.getRows(), yTest.getCols());
  out(yPred, muout, varSigmaOut);
  for(int j=0; j<getNumProcesses(); j++)
    {
      int error=0;
      for(int i=0; i<yTest.getRows(); i++)
	{
	  if(yPred.getVal(i, j)!=yTest.getVal(i, j))
	    error++;
	}
      double err=((double)error)/((double)yTest.getRows());
      err=err*100.0;
      cout << "Classification error on output " << j+1 << ": " << err << "%." << endl;
    }
}
void CProbitNoise::out(CMatrix& yOut, CMatrix& probOut, const CMatrix& muTest, const CMatrix& varSigmaTest) const
{
  assert(yOut.dimensionsMatch(probOut));
  out(yOut, muTest, varSigmaTest);
  likelihoods(probOut, muTest, varSigmaTest, yOut);
}

void CProbitNoise::out(CMatrix& yOut, const CMatrix& muTest, const CMatrix& varSigmaTest) const
{
  assert(yOut.dimensionsMatch(muTest));
  assert(muTest.dimensionsMatch(varSigmaTest));
  for(int j=0; j<yOut.getCols(); j++)
    {
      double b = bias.getVal(j);
      for(int i=0; i<yOut.getRows(); i++)	
	{
	  if(muTest.getVal(i, j)>-b)
	    yOut.setVal(1.0, i, j);
	  else
	    yOut.setVal(-1.0, i, j);
	}
    }
}
void CProbitNoise::likelihoods(CMatrix& L, const CMatrix& muTest, const CMatrix& varSigmaTest, 
				   const CMatrix& yTest) const
{
  assert(yTest.getCols()==getNumProcesses());
  assert(L.dimensionsMatch(muTest));
  assert(yTest.dimensionsMatch(muTest));
  assert(muTest.dimensionsMatch(varSigmaTest));
  double arg=0.0;
  double var=0.0;
  for(int j=0; j<muTest.getCols(); j++)
    {
      double b = bias.getVal(j);
      for(int i=0; i<muTest.getRows(); i++)
	{
	  arg = yTest.getVal(i, j)*(muTest.getVal(i, j) + b);
	  arg =arg/sqrt(varSigmaTest.getVal(i, j) + sigma2);
	  L.setVal(ndlutil::cumGaussian(arg), i, j);
	}
    }
}

double CProbitNoise::logLikelihood(const CMatrix& muTest, const CMatrix& varSigmaTest, 
				     const CMatrix& yTest) const
{
  assert(yTest.getCols()==getNumProcesses());
  assert(yTest.dimensionsMatch(muTest));
  assert(yTest.dimensionsMatch(varSigmaTest));
  double arg=0.0;
  double var=0.0;
  double L=0.0;
  for(int i=0; i<muTest.getRows(); i++)
    {
      for(int j=0; j<muTest.getCols(); j++)
	{
	  arg = muTest.getVal(i, j) + bias.getVal(j);
	  arg *= getTarget(i, j);
	  var = sqrt(varSigmaTest.getVal(i, j) + sigma2);
	  L += ndlutil::lnCumGaussian(arg/var);
	}
    }  
  return L;
}

CNcnmNoise::~CNcnmNoise()
{
}
void CNcnmNoise::initStoreage()
{
  if(isSplitGamma())
    setNumParams(getNumProcesses()+2);
  else
    setNumParams(getNumProcesses()+1);
  mu.resize(getNumData(), getNumProcesses());
  varSigma.resize(getNumData(), getNumProcesses());
  bias.resize(1, getNumProcesses());
  // sigmoid transforms on gamman and gammap.
  clearTransforms();
  addTransform(new CSigmoidTransform(), getNumParams()-1);
  if(isSplitGamma())
    addTransform(new CSigmoidTransform(), getNumParams()-2);
  sigma2 = 1e-6;
  width = 1.0;
  
  // sigma2 isn't treated as a parameter.
  setLogConcave(false);
  setSpherical(false);
  setMissing(true);

}
void CNcnmNoise::initVals()
{
  mu.zeros();
  varSigma.zeros();
  bias.zeros();

}
void CNcnmNoise::initNames()
{
  setType("ncnm");
  setNoiseName("Null Category");
  for(int j=0; j<getNumProcesses(); j++)
    setParamName("bias" + j, j);
}
void CNcnmNoise::initParams()
{
  double nClass1=0.0;
  double nClass2=0.0;
  double nMissing=0.0;
  for(int j=0; j<getNumProcesses(); j++)
    {
      for(int i=0; i<getNumData(); i++)
	{
	  if(y.getVal(i, j)==1.0)	    
	    nClass1++;
	  else if(y.getVal(i, j)==-1.0)
	    nClass2++;
	  else
	    nMissing++;
	}
      bias.setVal(ndlutil::invCumGaussian(nClass1/(nClass1+nClass2)), j);
    }
  gamman=nMissing/(double)getNumData();
  gammap = gamman;
  
}
ostream& CNcnmNoise::display(ostream& os)
{
  double b = 0.0;
  os << "Ncnm noise: " << endl;
  for(int j=0; j<bias.getCols(); j++)
    {
      b = bias.getVal(j);
      os << "Bias on process " << j << ": " << b << endl; 
    }
  os << "Missing label probability for -ve class: " << gamman << endl;
  os << "Missing label probability for +ve class: " << gammap << endl;
  return os;
}
void CNcnmNoise::setParam(double val, int index)
{
  assert(index>=0);
  assert(index<getNumParams());
  if(index<getNumProcesses())
    {
      bias.setVal(val, index);
      return;
    }
  if(index==getNumProcesses())
    {
      gamman=val;
      return;
    }
  if(index==getNumProcesses()+1)
    {
      gammap=val;
      return;
    }
}  
void CNcnmNoise::setParams(const CMatrix& params)
{
  assert(params.getCols()==getNumParams());
  assert(params.getRows()==1);
  int nProc = getNumProcesses();
  for(int j=0; j<nProc; j++)
    {
      bias.setVal(params.getVal(j), j);
    }
  gamman=params.getVal(nProc);
  if(isSplitGamma())
    gammap=params.getVal(nProc+1);
  else
    gammap=gamman;
}
double CNcnmNoise::getParam(const int index) const
{
  assert(index>=0);
  assert(index<getNumParams());
  if(index<getNumProcesses())
    return bias.getVal(index);
  if(index==getNumProcesses())
    return gamman;
  if(index==getNumProcesses()+1) // this only happens if the gammas aren't split.
    return gammap;
  return -1;
}
void CNcnmNoise::getParams(CMatrix& params) const
{
  assert(params.getCols()==getNumParams());
  assert(params.getRows()==1);
  int nProc=getNumProcesses();
  for(int j=0; j<nProc; j++)
    params.setVal(bias.getVal(j), j);
  params.setVal(gamman, nProc);
  if(isSplitGamma())
    params.setVal(gammap, nProc+1);
}
 
void CNcnmNoise::getGradParams(CMatrix& g) const
{
  assert(g.getCols()==getNumParams());
  assert(g.getRows()==1);
  double ggamman=0.0;
  double ggammap=0.0;
  double halfWidth = width/2.0;
  for(int j=0; j<getNumProcesses(); j++)
    {
      double gbias = 0.0;
      double b = bias.getVal(j);
      for(int i=0; i<getNumData(); i++)
	{
	  double muAdj = getMu(i, j)+b;
	  double c = 1/sqrt(sigma2+getVarSigma(i, j));
	  double targVal = getTarget(i, j);
	  if(targVal==-1.0)
	    {
	      muAdj+=halfWidth;
	      muAdj=muAdj*c;
	      gbias-= c*ndlutil::gradLnCumGaussian(-muAdj);
	      ggamman-=1.0/(1.0-gamman);
	    }
	  else if(targVal==1.0)
	    {
	      muAdj-=halfWidth;
	      muAdj*=c;
	      gbias+=c*ndlutil::gradLnCumGaussian(muAdj);
	      ggammap-=1.0/(1.0-gammap);
	    }
	  else
	    {
	      muAdj+=halfWidth;
	      double u=muAdj*c;
	      double uprime=(muAdj-width)*c;
	      double lndenom = ndlutil::lnCumGaussSum(-u, uprime, gamman, gammap);
	      double lnNumer1 = log(gamman) - ndlutil::HALFLOGTWOPI -.5*u*u;
	      double lnNumer2 = log(gammap) - ndlutil::HALFLOGTWOPI -.5*uprime*uprime;
	      double B1 = exp(lnNumer1-lndenom);
	      double B2 = exp(lnNumer2-lndenom);
	      gbias+=c*(B2-B1);
	      ggammap+=exp(ndlutil::lnCumGaussian(uprime)-lndenom);
	      ggamman+=exp(ndlutil::lnCumGaussian(-u)-lndenom);
	    }
	      
	}
      g.setVal(gbias, 0, j);
    }
  if(isSplitGamma())
    {
      g.setVal(ggamman, 0, getNumProcesses());
      g.setVal(ggammap, 0, getNumProcesses()+1);
    }
  else
    g.setVal(ggamman+ggammap, 0, getNumProcesses());

}
void CNcnmNoise::getGradInputs(double& gmu, double& gvs, const int i, const int j) const
{
  double b = bias.getVal(j);
  double c = 1.0/sqrt(sigma2+getVarSigma(i, j));
  double muAdj = getMu(i, j)+b;
  double targ = getTarget(i, j);
  double halfWidth = width/2.0;
  if(targ==-1.0)
    {
      muAdj+=halfWidth;
      muAdj*=c;
      gmu=-ndlutil::gradLnCumGaussian(-muAdj)*c;
      gvs=-.5*c*muAdj*gmu;
    }
  else if(targ==1.0)
    {
      muAdj-=halfWidth;
      muAdj*=c;
      gmu=ndlutil::gradLnCumGaussian(muAdj)*c;
      gvs=-.5*c*muAdj*gmu;
    }
  else // missing data
    {
      muAdj+=halfWidth;
      double u=c*muAdj;
      double uprime=(muAdj-width)*c;
      double lndenom=ndlutil::lnCumGaussSum(-u, uprime, gamman, gammap);
      double lnNumer1 = log(gamman) - ndlutil::HALFLOGTWOPI -.5*(u*u);
      double lnNumer2 = log(gammap) - ndlutil::HALFLOGTWOPI -.5*(uprime*uprime);
      double B1 = exp(lnNumer1 - lndenom);
      double B2 = exp(lnNumer2 - lndenom);
      gmu = c*(B2-B1);
      gvs = -.5*c*c*(uprime*B2-u*B1);
      
    }
  if(isnan(gmu))
    throw ndlexceptions::Error("gmu is NaN");
  if(isnan(gvs))
    throw ndlexceptions::Error("gvs is NaN");
}  
void CNcnmNoise::test(const CMatrix& muout, const CMatrix& varSigmaOut, const CMatrix& yTest) const
{
  assert(yTest.dimensionsMatch(muout));
  assert(muout.dimensionsMatch(varSigmaOut));
  assert(yTest.getCols()==getNumProcesses());
  CMatrix yPred(yTest.getRows(), yTest.getCols());
  out(yPred, muout, varSigmaOut);
  for(int j=0; j<getNumProcesses(); j++)
    {
      int missNum=0;
      int error=0;
      for(int i=0; i<yTest.getRows(); i++)
	{
	  if(yTest.getVal(i, j)==0.0 || isnan(yTest.getVal(i, j)))
	    missNum++;
	  else if(yPred.getVal(i, j)!=yTest.getVal(i, j))
	    error++;
	}
      if(missNum==yTest.getRows())
	cout << "No labels on output" << j+1 << "." << endl;
      double err=(double)error/(double)(yTest.getRows()-missNum);
      err=err*100.0;
      double miss=(double)missNum/(double)yTest.getRows();
      miss=miss*100.0;
      cout << "Classification error on output " << j+1 << ": " << err << "%, with " << miss << " missing values." << endl;
    }
}
  
void CNcnmNoise::out(CMatrix& yOut, const CMatrix& muTest, const CMatrix& varSigmaTest) const
{
  assert(yOut.dimensionsMatch(muTest));
  assert(muTest.dimensionsMatch(varSigmaTest));
  for(int j=0; j<yOut.getCols(); j++)
    {
      double b = bias.getVal(j);
      for(int i=0; i<yOut.getRows(); i++)	
	{
	  double muVal = muTest.getVal(i, j);
	  if(muVal>-b)
	    yOut.setVal(1.0, i, j);
	  else
	    yOut.setVal(-1.0, i, j);
	}
    }
}
void CNcnmNoise::out(CMatrix& yOut, CMatrix& probOut, const CMatrix& muTest, const CMatrix& varSigmaTest) const
{
  assert(yOut.dimensionsMatch(probOut));
  out(yOut, muTest, varSigmaTest);
  likelihoods(probOut, muTest, varSigmaTest, yOut);
}
void CNcnmNoise::likelihoods(CMatrix& L, const CMatrix& muTest, const CMatrix& varSigmaTest, 
				   const CMatrix& yTest) const
{
  assert(yTest.getCols()==getNumProcesses());
  assert(L.dimensionsMatch(muTest));
  assert(yTest.dimensionsMatch(muTest));
  assert(muTest.dimensionsMatch(varSigmaTest));
  double halfWidth = width/2.0;
  for(int j=0; j<muTest.getCols(); j++)
    {
      double b = bias.getVal(j);
      for(int i=0; i<muTest.getRows(); i++)
	{
	  double muAdj=muTest.getVal(i, j) + b;
	  double c=1/sqrt(sigma2+varSigmaTest.getVal(i, j));	  
	  double targVal=yTest.getVal(i, j);
	  if(targVal==1.0)
	    {
	      //muAdj-=halfWidth;
	      L.setVal(ndlutil::cumGaussian(muAdj*c), i, j);
	    }
	  else if(targVal==-1.0)
	    {
	      //muAdj+=halfWidth;
	      L.setVal(ndlutil::cumGaussian(-muAdj*c), i, j);
	    }
	}
    }
}

double CNcnmNoise::logLikelihood(const CMatrix& muTest, const CMatrix& varSigmaTest, 
				     const CMatrix& yTest) const
{
  assert(yTest.getCols()==getNumProcesses());
  assert(yTest.dimensionsMatch(muTest));
  assert(yTest.dimensionsMatch(varSigmaTest));
  double arg=0.0;
  double var=0.0;
  double L=0.0;
  double halfWidth = width/2.0;
  double logPosGamma = log(1.0-gammap);
  double logNegGamma = log(1.0-gamman);
  for(int j=0; j<muTest.getCols(); j++)    
    {
      double b=bias.getVal(j);
      for(int i=0; i<muTest.getRows(); i++)
	{
	  double muAdj = muTest.getVal(i, j) + b;
	  double c=1/sqrt(sigma2+varSigmaTest.getVal(i, j));
	  double targVal=yTest.getVal(i, j);
	  if(targVal==1.0)
	    {
	      muAdj-=halfWidth;
	      L+=ndlutil::lnCumGaussian(muAdj*c);
	      L+=logPosGamma;
	      
	    }
	  else if(targVal==-1.0)
	    {
	      muAdj+=halfWidth;
	      L+=ndlutil::lnCumGaussian(-muAdj*c);
	      L+=logNegGamma;
	    }
	  else // missing data.
	    {
	      muAdj+=halfWidth;
	      double u=muAdj*c;
	      double uprime=(muAdj-width)*c;
	      L+=ndlutil::lnCumGaussSum(-u, uprime, gamman, gammap);	      
	    }
	}
    }  
  return L;
}
void CNcnmNoise::writeParamsToStream(ostream& out) const
{
  out << "numParams=" << getNumParams() << endl;
  out << "numProcesses=" << getNumProcesses() << endl;
  if(isSplitGamma())
    out << "gammaSplit=1" << endl;
  else
    out << "gammaSplit=0" << endl;
  for(int i=0; i<getNumParams()-1; i++)
    out << getParam(i) << " ";
  out << getParam(getNumParams()-1) << endl;
}

void CNcnmNoise::readParamsFromStream(istream& in)
{
  string line;
  vector<string> tokens;
  getline(in, line);
  ndlstrutil::tokenise(tokens, line, "=");
  if(tokens.size()>2 || tokens[0]!="numParams")
    throw ndlexceptions::FileFormatError();
  int numPar=atol(tokens[1].c_str());
  tokens.clear();
  getline(in, line);
  ndlstrutil::tokenise(tokens, line, "=");
  if(tokens.size()>2 || tokens[0]!="numProcesses")
    throw ndlexceptions::FileFormatError();
  int numProc=atol(tokens[1].c_str());
  tokens.clear();
  getline(in, line);
  ndlstrutil::tokenise(tokens, line, "=");
  if(tokens.size()>2 || tokens[0]!="gammaSplit")
    throw ndlexceptions::FileFormatError();
  int gammaSplit=atol(tokens[1].c_str());
  if(gammaSplit!=0)
    setSplitGamma(true);
  else
    setSplitGamma(false);
      
  CMatrix par(1, numPar);
  tokens.clear();
  getline(in, line);
  ndlstrutil::tokenise(tokens, line, " ");
  for(int i=0; i<numPar; i++)
    par.setVal(atof(tokens[i].c_str()), i);
  setNumProcesses(numProc);
  if(numPar==getNumParams())
    setParams(par);
  else
    throw ndlexceptions::FileFormatError();
}
COrderedNoise::~COrderedNoise()
{
}
void COrderedNoise::initStoreage()
{
  setNumParams(getNumProcesses()+getNumCategories()-2);
  mu.resize(getNumData(), getNumProcesses());
  varSigma.resize(getNumData(), getNumProcesses());
  bias.resize(1, getNumProcesses());
  widths.resize(1, getNumCategories()-2);
  gwidth.resize(1, getNumCategories()-2);
  // sigmoid transforms on gamman and gammap.
  clearTransforms();
  for(int i=0; i<getNumCategories()-2; i++)
    addTransform(new CNegLogLogitTransform(), i+getNumProcesses());
  sigma2 = 0.1;
  
  // sigma2 isn't treated as a parameter.
  setLogConcave(true);
  setSpherical(false);
  setMissing(true);

}
void COrderedNoise::initVals()
{
  mu.zeros();
  varSigma.zeros();
  bias.zeros();
  widths.setVals(1/(getNumCategories()-2));

}
void COrderedNoise::initNames()
{
  setType("ordered");
  setNoiseName("Ordered Categorical");
  for(int j=0; j<getNumProcesses(); j++)
    setParamName("bias" + j, j);
  for(int j=0; j<getNumCategories()-2; j++)
    setParamName("width" + j, j+getNumProcesses());
}
void COrderedNoise::initParams()
{
  bias.deepCopy(meanCol(y));
}
ostream& COrderedNoise::display(ostream& os)
{
  double b = 0.0;
  os << "Ordered Categorical noise: " << endl;
  for(int j=0; j<bias.getCols(); j++)
    {
      b = bias.getVal(j);
      os << "Bias on process " << j << ": " << b << endl; 
    }
  for(int j=0; j<widths.getCols(); j++)
    {
      b = widths.getVal(j);
      os << "Width for category " << j << ": " << b << endl; 
    }
  return os;
}
void COrderedNoise::setParam(double val, int index)
{
  assert(index>=0);
  assert(index<getNumParams());
  if(index<getNumProcesses())
    {
      bias.setVal(val, index);
      return;
    }
  if(index>=getNumProcesses())
    {
      widths.setVal(val, index-getNumProcesses());
      return;
    }
}  
void COrderedNoise::setParams(const CMatrix& params)
{
  assert(params.getCols()==getNumParams());
  assert(params.getRows()==1);
  int nProc = getNumProcesses();
  for(int j=0; j<nProc; j++)
    {
      bias.setVal(params.getVal(j), j);
    }
  for(int j=0; j<getNumCategories()-2; j++)
    widths.setVal(params.getVal(j+getNumProcesses()), j);
}
double COrderedNoise::getParam(const int index) const
{
  assert(index>=0);
  assert(index<getNumParams());
  if(index<getNumProcesses())
    return bias.getVal(index);
  if(index>=getNumProcesses())
    return widths.getVal(index-getNumProcesses());
  return -1;
}
void COrderedNoise::getParams(CMatrix& params) const
{
  assert(params.getCols()==getNumParams());
  assert(params.getRows()==1);
  int nProc=getNumProcesses();
  for(int j=0; j<nProc; j++)
    params.setVal(bias.getVal(j), j);
  for(int j=0; j<getNumCategories()-2; j++)
    params.setVal(widths.getVal(j), j+getNumProcesses());
}
 
void COrderedNoise::getGradParams(CMatrix& g) const
{
  assert(g.getCols()==getNumParams());
  assert(g.getRows()==1);
  g.zeros();
  gwidth.zeros();
  for(int j=0; j<getNumProcesses(); j++)
    {
      double gbias = 0.0;
      double b = bias.getVal(j);
      for(int i=0; i<getNumData(); i++)
	{
	  double muAdj = getMu(i, j)+b;
	  int targVal = (int)getTarget(i, j);
	  assert(isnan(getTarget(i, j)) || getTarget(i, j)==(double)targVal);
	  double c = 1/sqrt(sigma2+getVarSigma(i, j));
	  if(targVal==0)
	    {
	      muAdj*=c;
	      gbias -= c*ndlutil::gradLnCumGaussian(-muAdj);
	    }
	  else if(targVal>0 && targVal<getNumCategories()-1)
	    {
	      for(int k=0; k<targVal-1; k++)
		muAdj -= widths.getVal(k);
	      double u = muAdj*c;
	      double uprime = (muAdj-widths.getVal(targVal-1))*c;
	      double B1 = ndlutil::gaussOverDiffCumGaussian(u, uprime, 1);
	      double B2 = ndlutil::gaussOverDiffCumGaussian(u, uprime, 2);
	      gbias += c*(B1-B2);
	      double addPart = c*B2;
	      for(int k=0; k<targVal; k++)
		gwidth.setVal(gwidth.getVal(k)+addPart, k);
	      if(targVal>1)
		{
		  addPart = c*B1;
		  for(int k=0; k<targVal-1; k++)
		    gwidth.setVal(gwidth.getVal(k)-addPart, k);	      
		}
	    }
	  else if(targVal==getNumCategories()-1)
	    {
	      for(int k=0; k<targVal-1; k++)
		muAdj -= widths.getVal(k);
	      muAdj*=c;
	      double addPart = c*ndlutil::gradLnCumGaussian(muAdj);
	      gbias += addPart;
	      if(getNumCategories()>2)
		for(int k=0; k<gwidth.getCols(); k++)
		  gwidth.setVal(gwidth.getVal(k)-addPart, k);
	    }
	  else if(isnan(getTarget(i, j)))
	    { /* do nothing*/  }
	  else
	    throw ndlexceptions::Error("This point should never be reached in COrderedNoise::getGradParams");
	}
      g.setVal(gbias, 0, j);
    }
  for(int j=0; j<getNumCategories()-2; j++)
    g.setVal(gwidth.getVal(j), j+getNumProcesses());
}
void COrderedNoise::getGradInputs(double& gmu, double& gvs, const int i, const int j) const
{
  double b = bias.getVal(j);
  double c = 1.0/sqrt(sigma2+getVarSigma(i, j));
  double muAdj = getMu(i, j)+b;
  int targ = (int)getTarget(i, j);
  assert(isnan(getTarget(i, j)) || getTarget(i, j)==(double)targ);
  if(targ==0)
    {
      muAdj*=c;
      gmu=-c*ndlutil::gradLnCumGaussian(-muAdj);
      gvs=-.5*gmu*c*muAdj;
    }
  
  else if(targ>0 && targ < getNumCategories() - 1)
    {
      for(int k=0; k<targ-1; k++)
	muAdj-=widths.getVal(k);
      double u = muAdj*c;
      double uprime = (muAdj - widths.getVal(targ-1))*c;
      double B1 = ndlutil::gaussOverDiffCumGaussian(u, uprime, 1);
      double B2 = ndlutil::gaussOverDiffCumGaussian(u, uprime, 2);
      gmu = c*(B1-B2);
      gvs = -.5*c*c*(u*B1 - uprime*B2);
    }
  else if (targ==getNumCategories()-1) // missing data
    {
      for(int k=0; k<targ-1; k++)
	muAdj-=widths.getVal(k);
      muAdj*=c;
      gmu = c*ndlutil::gradLnCumGaussian(muAdj);
      gvs = -.5*gmu*c*muAdj;
      
    }
  else if(isnan(getTarget(i, j)))
    { 
      gmu = 0.0;
      gvs = 0.0;
    }
  else
    throw ndlexceptions::Error("This point should never be reached in COrderedNoise::getGradInputs");
}  
void COrderedNoise::test(const CMatrix& muout, const CMatrix& varSigmaOut, const CMatrix& yTest) const
{
  assert(yTest.dimensionsMatch(muout));
  assert(muout.dimensionsMatch(varSigmaOut));
  assert(yTest.getCols()==getNumProcesses());
  CMatrix yPred(yTest.getRows(), yTest.getCols());
  out(yPred, muout, varSigmaOut);
  for(int j=0; j<getNumProcesses(); j++)
    {
      int error=0;
      for(int i=0; i<yTest.getRows(); i++)
	{
	  if(yPred.getVal(i, j)!=yTest.getVal(i, j))
	    error++;
	}
      double err=(double)error/(double)(yTest.getRows());
      err=err*100.0;
      cout << "Classification error on output " << j+1 << ": " << err << "% " << endl;
    }
}
  
void COrderedNoise::out(CMatrix& yOut, const CMatrix& muTest, const CMatrix& varSigmaTest) const
{
  assert(yOut.dimensionsMatch(muTest));
  assert(muTest.dimensionsMatch(varSigmaTest));
  for(int j=0; j<yOut.getCols(); j++)
    {
      double b = bias.getVal(j);
      for(int i=0; i<yOut.getRows(); i++)	
	{
	  double muVal = muTest.getVal(i, j)+b;
	  double tot = 0.0;
	  int counter = 1;
	  if(muVal<0)
	    {
	      yOut.setVal(0.0, i, j);
	      continue;
	    }
	  for(int k=0; k<widths.getCols(); k++)
	    if (muVal>tot)
	      {
		tot+=widths.getVal(i);
		counter++;
	      }
	    else
	      {
		yOut.setVal((double)counter, i, j);
		continue;
	      }
	}
      
    }
}
void COrderedNoise::out(CMatrix& yOut, CMatrix& probOut, const CMatrix& muTest, const CMatrix& varSigmaTest) const
{
  assert(yOut.dimensionsMatch(probOut));
  out(yOut, muTest, varSigmaTest);
  likelihoods(probOut, muTest, varSigmaTest, yOut);
}
void COrderedNoise::likelihoods(CMatrix& L, const CMatrix& muTest, const CMatrix& varSigmaTest, 
				   const CMatrix& yTest) const
{
  assert(yTest.getCols()==getNumProcesses());
  assert(L.dimensionsMatch(muTest));
  assert(yTest.dimensionsMatch(muTest));
  assert(muTest.dimensionsMatch(varSigmaTest));
  for(int j=0; j<muTest.getCols(); j++)
    {
      double b = bias.getVal(j);
      for(int i=0; i<muTest.getRows(); i++)
	{
	  double muAdj=muTest.getVal(i, j) + b;
	  double c=1/sqrt(sigma2+varSigmaTest.getVal(i, j));	  
	  int targVal=(int)yTest.getVal(i, j);
	  assert(isnan(getTarget(i, j)) || getTarget(i, j)==(double)targVal);
	  if(targVal==0)
	    {
	      L.setVal(ndlutil::cumGaussian(-muAdj*c), i, j);
	    }
	  else if(targVal>0 && targVal < getNumCategories()-1)
	    {
	      for(int k=0; k<targVal-1; k++)
		muAdj+=widths.getVal(k);
	      L.setVal(ndlutil::cumGaussian(muAdj*c)-ndlutil::cumGaussian((muAdj - widths.getVal(targVal-1))*c), i, j);
	    }
	  else if(targVal==getNumCategories()-1)
	    {
	      for(int k=0; k<targVal-1; k++)
		muAdj+=widths.getVal(k);
	      L.setVal(ndlutil::cumGaussian(muAdj*c), i, j);
	    }
	  else if(isnan(getTarget(i, j)))
	    {
	      L.setVal(1.0, i, j);
	    }
	  else
	    throw ndlexceptions::Error("This point should never be reached in COrderedNoise::getGradInputs");
	}
    }
}

double COrderedNoise::logLikelihood(const CMatrix& muTest, const CMatrix& varSigmaTest, 
				     const CMatrix& yTest) const
{
  assert(yTest.getCols()==getNumProcesses());
  assert(yTest.dimensionsMatch(muTest));
  assert(yTest.dimensionsMatch(varSigmaTest));
  double arg=0.0;
  double var=0.0;
  double L=0.0;
  for(int j=0; j<muTest.getCols(); j++)
    {
      double b = bias.getVal(j);
      for(int i=0; i<muTest.getRows(); i++)
	{
	  double muAdj=muTest.getVal(i, j) + b;
	  double c=1/sqrt(sigma2+varSigmaTest.getVal(i, j));	  
	  int targVal=(int)yTest.getVal(i, j);
	  assert(isnan(getTarget(i, j)) || getTarget(i, j)==(double)targVal);
	  if(targVal==0)
	    {
	      L+=ndlutil::lnCumGaussian(-muAdj*c);
	    }
	  else if(targVal>0 && targVal < getNumCategories()-1)
	    {
	      for(int k=0; k<targVal-1; k++)
		muAdj-=widths.getVal(k);
	      double u = muAdj*c;
	      double uprime = (muAdj - widths.getVal(targVal-1))*c;
	      L-=ndlutil::lnDiffCumGaussian(u, uprime);
	    }
	  else if(targVal==getNumCategories()-1)
	    {
	      for(int k=0; k<targVal-1; k++)
		muAdj-=widths.getVal(k);
	      L+=ndlutil::lnCumGaussian(muAdj*c);
	    }
	  else if(isnan(getTarget(i, j)))
	    {/*do nothing*/}
	  else
	    throw ndlexceptions::Error("This point should never be reached in COrderedNoise::getGradInputs");

	}
    }  
  return L;
}
void COrderedNoise::writeParamsToStream(ostream& out) const
{
  out << "numParams=" << getNumParams() << endl;
  out << "numProcesses=" << getNumProcesses() << endl;
  out << "numCategories=" << getNumCategories() << endl;
  for(int i=0; i<getNumParams()-1; i++)
    out << getParam(i) << " ";
  out << getParam(getNumParams()-1) << endl;
}

void COrderedNoise::readParamsFromStream(istream& in)
{
  string line;
  vector<string> tokens;
  getline(in, line);
  ndlstrutil::tokenise(tokens, line, "=");
  if(tokens.size()>2 || tokens[0]!="numParams")
    throw ndlexceptions::FileFormatError();
  int numPar=atol(tokens[1].c_str());
  tokens.clear();
  getline(in, line);
  ndlstrutil::tokenise(tokens, line, "=");
  if(tokens.size()>2 || tokens[0]!="numProcesses")
    throw ndlexceptions::FileFormatError();
  int numProc=atol(tokens[1].c_str());
  tokens.clear();
  getline(in, line);
  ndlstrutil::tokenise(tokens, line, "=");
  if(tokens.size()>2 || tokens[0]!="numCategories")
    throw ndlexceptions::FileFormatError();
  int numCat=atol(tokens[1].c_str());
      
  CMatrix par(1, numPar);
  tokens.clear();
  getline(in, line);
  ndlstrutil::tokenise(tokens, line, " ");
  for(int i=0; i<numPar; i++)
    par.setVal(atof(tokens[i].c_str()), i);
  setNumProcesses(numProc);
  setNumCategories(numCat);
  if(numPar==getNumParams())
    setParams(par);
  else
    throw ndlexceptions::FileFormatError();
}

void writeNoiseToStream(const CNoise& noise, ostream& out)
{
  out << "noiseVersion=" << NOISEVERSION << endl;
  out << "type=" << noise.getType() << endl;
  noise.writeParamsToStream(out);
}

CNoise* readNoiseFromStream(istream& in)
{
  CNoise* pnoise;
  string line;
  vector<string> tokens;
  // first line is version info.
  getline(in, line);
  ndlstrutil::tokenise(tokens, line, "=");
  if(tokens.size()>2 || tokens[0]!="noiseVersion")
    throw ndlexceptions::FileFormatError();
  if(tokens[1]!="0.1")
    throw ndlexceptions::FileVersionError();
  // next line is type.
  tokens.clear();
  getline(in, line);
  ndlstrutil::tokenise(tokens, line, "=");
  if(tokens.size()>2 || tokens[0]!="type")
    throw ndlexceptions::FileFormatError();
  string type=tokens[1];
  if(type=="probit")
    pnoise = new CProbitNoise();
  else if(type=="ncnm")
    pnoise = new CNcnmNoise();
  else if(type=="gaussian")
    pnoise = new CGaussianNoise();
  else if(type=="ordered")
    pnoise = new COrderedNoise();
  else if(type=="scale")
    pnoise = new CScaleNoise();
  else
    throw ndlexceptions::Error("Unknown Noise Type Specified");

  pnoise->readParamsFromStream(in);
  return pnoise;
}
