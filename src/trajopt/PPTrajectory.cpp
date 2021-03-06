#include "trajopt/PPTrajectory.h"

namespace trajopt {

PPTrajectory::PPTrajectory(
		Eigen::VectorXd sample_times,
		int num_vars,
		int degree,
		int continuity_degree
		) :
	sample_times_(sample_times),
	num_vars_(num_vars),
	degree_(degree),
	continuity_degree_(continuity_degree)
{
	// One polynomial for each time slice
	for (int i = 0;	i < sample_times.size(); ++i)
	{
		// d + 1 coefficients for each variable
		coeffs_.push_back(prog_.NewContinuousVariables(num_vars, degree + 1, "C"));
	}

	// Add continuity constraints
	for (int s = 0; s < sample_times.size() - 1; ++s)
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
		for (int s = 0; s < sample_times_.size() - 1; ++s)
		{
			auto Q = Eigen::MatrixXd::Identity(num_vars_, num_vars_);
			auto b = Eigen::VectorXd::Zero(num_vars_);
			prog_.AddQuadraticCost(Q, b, coeffs_[s](Eigen::all, Eigen::last));
		}
	}
}

Eigen::VectorX<drake::symbolic::Expression> PPTrajectory::eval_symbolic(const double t)
{
	return PPTrajectory::eval(t, 0);
}


Eigen::VectorX<drake::symbolic::Expression> PPTrajectory::eval_symbolic(
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
	while (s < sample_times_.size() - 1 && t >= sample_times_[s + 1]) ++s;

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

Eigen::VectorXd PPTrajectory::eval(const double t)
{
	return eval(t, 0);
}

// TODO rewrite this somehow?
Eigen::VectorXd PPTrajectory::eval(const double t, const int derivative_order)
{
	if (derivative_order > degree_)
	{
		auto zero_vec = Eigen::VectorXd::Zero(num_vars_);
		return zero_vec;
	}

	// Find the right polynomial segment
	int s = 0;
	while (s < sample_times_.size() - 1 && t >= sample_times_[s + 1]) ++s;

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

void PPTrajectory::add_constraint(
		const double t, const int derivative_order, Eigen::VectorXd lb
		)
{
	add_constraint(t, derivative_order, lb, lb);
}

void PPTrajectory::add_constraint(
		const double t, const int derivative_order, Eigen::VectorXd lb, Eigen::VectorXd ub
		)
{
	assert(derivative_order <= degree_);
	Eigen::VectorX<drake::symbolic::Expression> val = eval_symbolic(t, derivative_order);
	prog_.AddLinearConstraint(val, lb, ub);
}

void PPTrajectory::generate()
{
	result_ = Solve(prog_);
	//assert(result_.is_success());
	std::cout << "Solver id: " << result_.get_solver_id() << std::endl;
	std::cout << "Found solution: " << result_.is_success() << std::endl;
	std::cout << "Solution result: " << result_.get_solution_result() << std::endl;
}

} // Namespace trajopt
