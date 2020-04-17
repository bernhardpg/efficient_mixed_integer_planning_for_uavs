#include "trajectory_optimization.h"


namespace trajopt
{

MISOSProblem::MISOSProblem(
		Eigen::VectorXd sample_times,
		int num_vars,
		int degree,
		int continuity_degree
		) :
	sample_times_(sample_times),
	num_vars_(num_vars),
	degree_(degree),
	continuity_degree_(continuity_degree),
	num_traj_segments_(sample_times.size())
{
	t_ = prog_.NewIndeterminates(1, 1, "t");

	// One polynomial for each time slice
	for (int i = 0;	i < num_traj_segments_; ++i)
	{
		// d + 1 coefficients for each variable
		coeffs_.push_back(prog_.NewContinuousVariables(num_vars, degree + 1, "C"));
	}

	// Plan
	// 1. Add m list of monomials
	// 2. Calculate polynomial from this
	// 3. Calculate derivatives of the polynomial
	// 4. Get the coefficients of the derivatives
	// 5. Do this for every derivative order and for every traj segment
	// then
	// 6. Constrain initial and final value
	// 7. Enforce continuity on the derivatives of the polynomial


	/*
	// Add continuity constraints
	for (int s = 0; s < num_traj_segments_ - 1; ++s)
	{
		double time_rel = sample_times[s + 1] - sample_times[s];
		auto curr_coeffs = coeffs_[s];

		for (int var = 0; var < num_vars; ++var)
		{
			// For every derivative degree we enforce continuity on
			for (int derivative_deg = 0; derivative_deg < continuity_degree + 1; ++derivative_deg)
			{
				drake::symbolic::Expression left_val;
				for (int deg = derivative_deg; deg < degree_ + 1; ++deg)
				{
					// Calculate (d/dt)**derivatice_deg of P_j(1)
					left_val += (curr_coeffs(var, deg) * pow(time_rel, deg - derivative_deg)
						* factorial(deg) / factorial(deg - derivative_deg));
				}
				// Calculate (d/dt)**derivatice_deg of P_j+1(0)
				// (Note: only the constant term remains)
				drake::symbolic::Expression right_val = (coeffs_[s + 1](var, derivative_deg)
					* factorial(derivative_deg));
				prog_.AddLinearConstraint(left_val == right_val);
			}
		}

		// Add cost to minimize highest order terms
		for (int s = 0; s < num_traj_segments_ - 1; ++s)
		{
			auto Q = Eigen::MatrixXd::Identity(num_vars_, num_vars_);
			auto b = Eigen::VectorXd::Zero(num_vars_);
			prog_.AddQuadraticCost(Q, b, coeffs_[s](Eigen::all, Eigen::last));
		}
	}
	*/

}

Eigen::VectorX<drake::symbolic::Expression> MISOSProblem::eval_symbolic(const double t)
{
	return MISOSProblem::eval(t, 0);
}


Eigen::VectorX<drake::symbolic::Expression> MISOSProblem::eval_symbolic(
		const double t, const int derivative_order
		)
{
	if (derivative_order > degree_)
	{
		auto zero_vec = Eigen::VectorX<drake::symbolic::Expression>::Zero(num_vars_);
		return zero_vec;
	}

	// Find the right polynomial segment
	int s = 0;
	while (s < num_traj_segments_ - 1 && t >= sample_times_[s + 1]) ++s;

	const double time_rel = t - sample_times_[s];

	auto coeffs = coeffs_[s];

	const int derivative_deg = derivative_order;
	Eigen::VectorX<drake::symbolic::Expression> val(num_vars_);

	for (int var = 0; var < num_vars_; ++var)
	{
		drake::symbolic::Expression curr_val;
		for (int deg = derivative_deg; deg < degree_ + 1; ++deg)
		{
			curr_val += (coeffs(var, deg) * pow(time_rel, deg - derivative_deg)
				* factorial(deg) / factorial(deg - derivative_deg));
		}
		val(var) = curr_val;
	}
	return val;
}

Eigen::VectorXd MISOSProblem::eval(const double t)
{
	return eval(t, 0);
}

// TODO rewrite this somehow?
Eigen::VectorXd MISOSProblem::eval(const double t, const int derivative_order)
{
	if (derivative_order > degree_)
	{
		auto zero_vec = Eigen::VectorXd::Zero(num_vars_);
		return zero_vec;
	}

	// Find the right polynomial segment
	int s = 0;
	while (s < num_traj_segments_ - 1 && t >= sample_times_[s + 1]) ++s;

	const double time_rel = t - sample_times_[s];

	Eigen::MatrixXd coeffs = result_.GetSolution(coeffs_[s]);

	const int derivative_deg = derivative_order;
	Eigen::VectorXd val(num_vars_);

	for (int var = 0; var < num_vars_; ++var)
	{
		double curr_val = 0;
		for (int deg = derivative_deg; deg < degree_ + 1; ++deg)
		{
			curr_val += (coeffs(var, deg) * pow(time_rel, deg - derivative_deg))
				* factorial(deg) / factorial(deg - derivative_deg);
		}
		val(var) = curr_val;
	}
	return val;
}

void MISOSProblem::add_constraint(
		const double t, const int derivative_order, Eigen::VectorXd lb
		)
{
	add_constraint(t, derivative_order, lb, lb);
}

void MISOSProblem::add_constraint(
		const double t, const int derivative_order, Eigen::VectorXd lb, Eigen::VectorXd ub
		)
{
	assert(derivative_order <= degree_);
	Eigen::VectorX<drake::symbolic::Expression> val = eval_symbolic(t, derivative_order);
	prog_.AddLinearConstraint(val, lb, ub);
}

void MISOSProblem::generate()
{
	result_ = Solve(prog_);
	//assert(result_.is_success());
	std::cout << "Solver id: " << result_.get_solver_id() << std::endl;
	std::cout << "Found solution: " << result_.is_success() << std::endl;
	std::cout << "Solution result: " << result_.get_solution_result() << std::endl;
}

void MISOSProblem::add_region_constraint(int polynomial, Eigen::MatrixXd A, Eigen::VectorXd b)
{
	// TODO continue here! Something about time variable t
	// not working. Check out SOS drake
	drake::symbolic::Expression q;
	for (int i = 0; i < A.rows(); ++i)
	{
		auto ai = A(i, Eigen::all);
		auto bi = b(i);

		//std::cout << "ai" << std::endl << ai << std::endl << "bi" << std::endl << bi << std::endl;

		for (int d = 0; d < degree_ + 1; ++d)
		{
			auto expr = ai.dot(coeffs_[i](Eigen::all, d));
			q -= expr;
		}
		q += bi;
		//std::cout << "q:" << std::endl << q << std::endl << std::endl;

		prog_.AddLinearConstraint(q >= 0);
	}
}

int factorial(int n)
{
	return (n==1 || n==0) ? 1: n * factorial(n - 1);
}
} // Namespace trajopt
