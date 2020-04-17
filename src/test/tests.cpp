#include "test/tests.h"

void test_iris()
{
	std::cout << "Testing IRIS" << std::endl;

	// Create bounding box
	Eigen::MatrixXd A(4,2);
	A << -1, 0,
				0, -1,
				1, 0,
				0, 1;
	Eigen::VectorXd b(4);
	b << 0, 0, 5, 5;
	iris::Polyhedron bounds(A,b);

	iris::IRISProblem problem(2);
	problem.setBounds(bounds);
	problem.setSeedPoint(Eigen::Vector2d(0.1, 0.1));

	std::vector<Eigen::MatrixXd> obstacles;
	Eigen::MatrixXd obs(4,2);
	obs << 2, 0,
				 2, 2,
				 3, 2,
				 3, 0;
	problem.addObstacle(obs.transpose());
	obstacles.push_back(obs);
	obs << -1, 0,
				 -1, 2,
				  0, 2,
					0.2, 0;
	problem.addObstacle(obs.transpose());
	obstacles.push_back(obs);

  iris::IRISOptions options;
  iris::IRISRegion region = inflate_region(problem, options);

	iris::Polyhedron solution = region.getPolyhedron();
	
	auto points = solution.generatorPoints();
	plot_convex_hull(points);
	plot_obstacles(obstacles);
}


void test_polynomial_trajectory()
{
	const int tf = 3;
	auto sample_times = Eigen::VectorXd::LinSpaced(6, 0, tf);
	const int num_vars = 2;
	const int degree = 5;
	const int continuity_degree = 4;

	trajopt::PPTrajectory traj(sample_times, num_vars, degree, continuity_degree);

	Eigen::Matrix<double, 5, 2> points;
	points << 0.0, 0.0,
						1.0, 2.0,
						5.0, 1.5,
						3.0, 5.0,
						6.0, 3.0;

	traj.add_constraint(0.0, 0, points(0, Eigen::all));
	traj.add_constraint(0.0, 1, points(0, Eigen::all));
	traj.add_constraint(0.0, 2, points(0, Eigen::all));
	traj.add_constraint(1.0, 0, points(1, Eigen::all));
	traj.add_constraint(1.5, 0, points(2, Eigen::all));
	traj.add_constraint(2.5, 0, points(3, Eigen::all));
	traj.add_constraint(tf, 0, points(4, Eigen::all));
	traj.add_constraint(tf, 1, points(0, Eigen::all));
	traj.add_constraint(tf, 2, points(0, Eigen::all));

	traj.generate();

	const double delta_t = 0.01;
	int N = (int)(tf / delta_t);

	std::vector<double> x;
	std::vector<double> y;

	for (int i = 0; i < N; ++i)
	{
		double t = 0.0 + delta_t * i;
		x.push_back(traj.eval(t)(0));
		y.push_back(traj.eval(t)(1));
	}

	auto x_vec = points(Eigen::all, 0);
	auto y_vec = points(Eigen::all, 1);
	std::vector<double> points_x_coord(x_vec.data(), x_vec.data() + x_vec.rows());
	std::vector<double> points_y_coord(y_vec.data(), y_vec.data() + y_vec.rows());

	typedef	std::unordered_map<std::string, std::string> string_map;
	plt::scatter(points_x_coord, points_y_coord, 20, string_map({{"color","red"}}));

	std::vector<double> sample_times_x;
	std::vector<double> sample_times_y;
	for (int i = 0; i < sample_times.size(); ++i)
	{
			sample_times_x.push_back(traj.eval(sample_times[i])(0));
			sample_times_y.push_back(traj.eval(sample_times[i])(1));
	}

	plt::scatter(sample_times_x, sample_times_y, 10);

	plt::plot(x, y);

	plt::show();

	return;
}


void test_mathematical_program()
{
	drake::solvers::MathematicalProgram	prog;

	Eigen::Matrix2d A;
	A << 1, 2, 3, 4;
	Eigen::Vector2d b;
	b << 10, 10;

	std::cout << A(Eigen::all,Eigen::last) << std::endl;

	auto state = prog.NewContinuousVariables(2, "state");
	prog.AddLinearConstraint((A * state).array() >= b.array());
	prog.AddCost(state.transpose() * state);

	drake::solvers::MathematicalProgramResult result = Solve(prog);
	assert(result.is_success());
	std::cout << "Result: " << std::endl << result.GetSolution() << std::endl;

	return;
}