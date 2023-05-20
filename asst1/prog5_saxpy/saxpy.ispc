
export void saxpy_ispc(uniform int N,
                       uniform float scale,
                            uniform float X[],
                            uniform float result[])
{
    foreach (i = 0 ... N) { 
       result[i] = scale * X[i]* X[i]* X[i] ;
        
    }
}

task void saxpy_ispc_task(uniform int N,
                               uniform int span,
                               uniform float scale,
                               uniform float X[], 
                               uniform float result[])
{

    uniform int indexStart = taskIndex * span;
    uniform int indexEnd = min(N, indexStart + span);

    foreach (i = indexStart ... indexEnd) {
       result[i] = scale * X[i]* X[i]* X[i] ;
        
    }
}

export void saxpy_ispc_withtasks(uniform int N,
                               uniform float scale,
                               uniform float X[],
                               uniform float result[])
{

    uniform int span = N / 32;  // 64 tasks

    launch[N/span] saxpy_ispc_task(N, span, scale, X, result);
}
