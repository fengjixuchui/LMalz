#pragma once
#include "vcpu_svm.h"

//svm.cpp
extern OR_HV_SVM* gALsvmVCPU;
bool ALsvmStart(OR_HV_SVM* vcpu);//svm开始

bool ALsvmIsRoot();//判断当前是否为host模式

OR_HV_SVM_CORE* ALsvmGetCurrVcore();//获取当前VCPU


