#pragma once

#include <drake/common/symbolic.h>
#include <drake/solvers/mathematical_program.h>
#include <drake/solvers/solve.h>
#include <drake/common/trajectories/piecewise_polynomial.h>
#include <iostream>
#include <Eigen/Dense>


namespace trajopt
{
	typedef Eigen::MatrixX<drake::symbolic::Expression> coeff_matrix_t;

	class MISOSProblem
	{
		public:
			MISOSProblem(
					const int num_traj_segments,
					const int num_vars,
					const int degree,
					const int continuity_degree,
					Eigen::VectorX<double> init_cond,
					Eigen::VectorX<double> final_cond
					);


			void add_region_constraint(Eigen::MatrixXd A, Eigen::VectorXd b, int segment_number);
			void generate();
			Eigen::VectorX<double> eval(double t);

		private:
			const int num_vars_;
			const int degree_;
			const int continuity_degree_;
			const int num_traj_segments_;
			const double vehicle_radius_;

			drake::symbolic::Variable t_;
			std::vector<coeff_matrix_t> coeffs_;
			std::vector<std::vector<coeff_matrix_t>> coeffs_d_;
			Eigen::VectorX<drake::symbolic::Expression> m_; // Vector of monomial basis functions
			drake::solvers::MathematicalProgram prog_;

			drake::solvers::MathematicalProgramResult result_;
			Eigen::MatrixX<drake::symbolic::Polynomial> polynomials_;

			Eigen::VectorX<drake::symbolic::Expression> get_coefficients(drake::symbolic::Polynomial p);
	};
	int factorial(int n);
	}
