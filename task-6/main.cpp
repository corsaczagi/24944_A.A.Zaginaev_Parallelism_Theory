#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <chrono>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

void initialize_boundaries(double* A, int m, int n)
{
    const double tl = 10.0; // top-left
    const double tr = 20.0; // top-right
    const double br = 30.0; // bottom-right
    const double bl = 20.0; // bottom-left

    // левая граница
    #pragma acc parallel loop present(A)
    for (int i = 0; i < m; i++)
    {
        double t = static_cast<double>(i) / (m - 1);
        A[i] = tl * (1.0 - t) + bl * t;
    }

    // правая граница
    #pragma acc parallel loop present(A)
    for (int i = 0; i < m; i++)
    {
        double t = static_cast<double>(i) / (m - 1);
        A[i + (n - 1) * m] = tr * (1.0 - t) + br * t;
    }

    // верхняя граница
    #pragma acc parallel loop present(A)
    for (int j = 1; j < n - 1; j++)
    {
        double t = static_cast<double>(j) / (n - 1);
        A[j * m] = tl * (1.0 - t) + tr * t;
    }

    // нижняя граница
    #pragma acc parallel loop present(A)
    for (int j = 1; j < n - 1; j++)
    {
        double t = static_cast<double>(j) / (n - 1);
        A[(m - 1) + j * m] = bl * (1.0 - t) + br * t;
    }
}

void initialize(double* A, double* Anew, int m, int n)
{
    #pragma acc parallel loop present(A,Anew)
    for (int i = 0; i < m * n; i++)
    {
        A[i] = 0.0;
        Anew[i] = 0.0;
    }

    initialize_boundaries(A, m, n);
    initialize_boundaries(Anew, m, n);
}

double jacobi_step(double* A, double* Anew, int m, int n)
{
    double error = 0.0;

    #pragma acc parallel loop collapse(2) reduction(max:error) present(A,Anew)
    for (int j = 1; j < n - 1; j++)
    {
        for (int i = 1; i < m - 1; i++)
        {
            int idx = i + j * m;

            double new_val =
                0.25 *
                (
                    A[idx - 1] +
                    A[idx + 1] +
                    A[idx - m] +
                    A[idx + m]
                );

            double diff = fabs(new_val - A[idx]);

            if (diff > error)
                error = diff;

            Anew[idx] = new_val;
        }
    }

    return error;
}

void printGrid(const double* A, int m, int n)
{
    std::cout << std::fixed << std::setprecision(4);

    for (int j = n - 1; j >= 0; --j)
    {
        for (int i = 0; i < m; ++i)
        {
            std::cout << std::setw(10)
                      << A[i + j * m]
                      << " ";
        }
        std::cout << "\n";
    }
}

int main(int argc, char* argv[])
{
    int size = 256;
    double eps = 1e-6;
    int max_iter = 1000000;

    po::options_description desc("Options");

    desc.add_options()
        ("help,h", "help")
        ("size,s", po::value<int>(&size)->default_value(256),
         "grid size")
        ("eps,e", po::value<double>(&eps)->default_value(1e-6),
         "precision")
        ("iter,i", po::value<int>(&max_iter)->default_value(1000000),
         "max iterations");

    po::variables_map vm;

    po::store(
        po::parse_command_line(argc, argv, desc),
        vm);

    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 0;
    }

    const int m = size;
    const int n = size;

    std::vector<double> A(m * n);
    std::vector<double> Anew(m * n);

    double* A_ptr = A.data();
    double* Anew_ptr = Anew.data();

    #pragma acc enter data copyin(A_ptr[0:m*n],Anew_ptr[0:m*n])

    initialize(A_ptr, Anew_ptr, m, n);

    if (size == 10 || size == 13)
    {
        #pragma acc update self(A_ptr[0:m*n])

        std::cout << "\nInitial grid:\n";
        printGrid(A_ptr, m, n);
    }

    auto start =
        std::chrono::high_resolution_clock::now();

    double error = 1.0;
    int iter = 0;

    while (error > eps && iter < max_iter)
    {
        error = jacobi_step(
            A_ptr,
            Anew_ptr,
            m,
            n);

        // обмен указателей вместо копирования
        double* tmp = A_ptr;
        A_ptr = Anew_ptr;
        Anew_ptr = tmp;

        ++iter;

        if (iter % 10000 == 0)
        {
            std::cout
                << "iter = "
                << iter
                << " error = "
                << error
                << std::endl;
        }
    }

    auto stop =
        std::chrono::high_resolution_clock::now();

    double elapsed =
        std::chrono::duration<double>(
            stop - start).count();

    std::cout << "\n=============================\n";
    std::cout << "Iterations : " << iter << "\n";
    std::cout << "Final error: " << error << "\n";
    std::cout << "Time       : " << elapsed << " sec\n";
    std::cout << "=============================\n";

    if (size == 10 || size == 13)
    {
        #pragma acc update self(A_ptr[0:m*n])

        std::cout << "\nFinal grid:\n";
        printGrid(A_ptr, m, n);
    }

    #pragma acc exit data delete(A_ptr[0:m*n],Anew_ptr[0:m*n])

    return 0;
}