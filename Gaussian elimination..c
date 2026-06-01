//
//  main.c
//  мгу
//
//  Created by Гнездилов Денис
#include <stdio.h>
#include <math.h>
int main(void) {
    int n;
    double m[1000][1000];
    scanf("%d", &n);
    for(int i = 0; i < n; i++){
        for(int j = 0; j < n; j++) {
            scanf("%lf", &m[i][j]);
        }
    }
    int sgn = 1;
    for(int c = 0; c < n; c++) {
        int mx = c;
        for(int r = c+1; r < n; r++){
            if(fabs(m[r][c]) > fabs(m[mx][c])){
                mx = r;
            }
        }
        if(mx != c) {
            for(int j = 0; j < n; j++) {
                double t = m[c][j];
                m[c][j] = m[mx][j];
                m[mx][j] = t;
            }
            sgn = -sgn;
        }
        if(fabs(m[c][c]) < 1e-9) {
            return 0;
        }
        for(int r = c+1; r < n; r++) {
            double f = m[r][c] / m[c][c];
            for(int j = c; j < n; j++){
                m[r][j] -= f * m[c][j];
            }
        }
    }
    double det = sgn;
    for(int i = 0; i < n; i++){
        det *= m[i][i];
    }
    printf("%f\n", det);
    return 0;
}
