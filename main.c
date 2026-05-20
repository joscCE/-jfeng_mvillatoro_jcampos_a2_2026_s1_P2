#include <stdio.h>


//tamano de la matriz

#define HEIGHT 10 
#define WIDTH 10

//estos son los coeficientes de difusion

float Da = 1.0f;
float Db = 0.5f;

//diferencial de tiempo

float dt = 1.0f;

//entrega y disminucion de sustancias

float feed = 0.055f;
float kill = 0.062f;


//matrices iniciales
float A[HEIGHT][WIDTH];
float B[HEIGHT][WIDTH];


//matrices siguientes para despues hacer swap
float A_next[HEIGHT][WIDTH];
float B_next[HEIGHT][WIDTH];

//calculo del laplaciano, nos da la difusion en diferentes puntos
float laplacian(int x, int y, float M[HEIGHT][WIDTH]){
    return
        -1.0f * M[x][y] //centro

        +0.2f * ( //lados
            M[x+1][y] +
            M[x-1][y] +
            M[x][y+1] +
            M[x][y-1]
        )
        +0.05f * ( //diagonales
            M[x+1][y+1] +
            M[x+1][y-1] +
            M[x-1][y+1] +
            M[x-1][y-1]
        );
}


int main(){

    // Inicializar lleno de liquido A
    for(int i=0;i<HEIGHT;i++){
        for(int j=0;j<WIDTH;j++){
            A[i][j]=1.0f;
            B[i][j]=0.0f;

            A_next[i][j]=1.0f;
            B_next[i][j]=0.0f;
        }
    }


    B[HEIGHT/2][WIDTH/2]=1.0f; //una gotita de B en el centro, para que pase algo 


\
    for(int t=0; t<100; t++){ //for para el paso del tiempo

        for(int i=1;i<HEIGHT-1;i++){
            for(int j=1;j<WIDTH-1;j++){

                float reaction =
                    A[i][j] * B[i][j] * B[i][j];

                float dA =
                    Da * laplacian(i,j,A)
                    - reaction
                    + feed*(1.0f-A[i][j]);

                float dB =
                    Db * laplacian(i,j,B)
                    + reaction
                    - B[i][j]*(kill+feed);

                A_next[i][j] =
                    A[i][j] + dA*dt;

                B_next[i][j] =
                    B[i][j] + dB*dt;
            }
        }

        // se cambian las matrices, para dar la evolucion
        for(int i=0;i<HEIGHT;i++){
            for(int j=0;j<WIDTH;j++){
                A[i][j]=A_next[i][j];
                B[i][j]=B_next[i][j];
            }
        }
    }

    return 0;
}