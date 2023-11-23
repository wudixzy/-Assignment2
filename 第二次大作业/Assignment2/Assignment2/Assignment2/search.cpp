#include "search.h"
#include "inference.h"
#include <iostream>

// 目前采用了最简单的选择方式：按照variables的顺序选择。
Queen* search::selectUnassignedVariable(Csp& csp)
{
	for (Queen* q : csp.variables)
	{
		if (q->position == Position::getUnassigned())
		{
			return q;
		}
	}

	return NULL;
}

// 最简单的顺序：按照variables本身domain的顺序。
std::vector<Position> search::orderDomainValues(Queen* var, std::vector<Queen*>& assignment, Csp& csp)
{
	return var->domain;
}

/*
 * Inferencing can eliminate the domain of variables according to current assignment.
 * This function return inferences which can be added to assignment.
 * Inferences means the variables whose domain size has been eliminated to 1 after inferencing.
*/
std::vector<Queen*> search::makeInference(Csp& csp, Queen* var, Position value)
{
	std::vector<Queen*> result;
	inference::ac3(csp);
	for (Queen* q : csp.variables)
	{
		if (q->domain.empty()) return std::vector<Queen*>({NULL});
		if (q->domain.size() == 1 && q->position == Position::getUnassigned())
		{
			q->assign(q->domain[0]);
			result.push_back(q);
		}
	}

	return result;
}
// an inferences which contains only one null pointer indicates failure.
bool search::failed(std::vector<Queen*>& inferences)
{
	if (inferences.size() == 1)
	{
		if (inferences[0] == NULL) return true;
	}

	return false;
}

bool search::isSolution(Csp& csp, std::vector<Queen*>& solution)
{
	bool result = true;

	for (Queen* q : solution)
	{
		if (getConflicts(csp, q->position) > 0) result = false;
	}

	return result;
}

// remove assigned value and inferences from assignment
void search::refresh(std::vector<Queen*>& assignment)
{
	auto it = assignment.begin();
	while (it != assignment.end())
	{
		Queen* current = *it;
		if (current->position == Position::getUnassigned()) it = assignment.erase(it);
		else it++;
	}
}

std::vector<Queen*> search::backtrackingSearch(Csp& csp)
{
	return backtrack(std::vector<Queen*>(), csp);
}

std::vector<Queen*> search::backtrack(std::vector<Queen*> assignment, Csp& csp)
{
	/*
	 * TODO
	 * Algorithm (Reference: Figure 6.5):
	 function BACKTRACK(assignment, csp) returns a solution, or failure
		if assignment is complete then return assignment (use this condition: assignment.size() == csp.variables.size())
		var<-SELECT-UNASSIGNED-VARIABLE(csp)
		for each value in ORDER-DOMAIN-VALUES(var, assignment, csp) do
			record csp state # csp.recode() require two variables, you need to create two local variables to store the state
			if value is consistent with assignment then
				assign value to var	# use var->assign(value)
				add var to assignment
				inferences<-INFERENCE(csp, var, value)	# use makeInference function here
				if inferences != failure then
					add inferences to assignment
					result<-BACKTRACK(assignment, csp)
					if result != failure then
						return result
			recover csp state (csp.recover)
			remove {var = value} and inferences from assignment # use refresh(assignment)
		return failure
	 */
	std::vector<Queen*>FAILURE({ NULL });
	std::vector<Queen*>result;
	if (assignment.size() == csp.variables.size())return assignment;
	Queen* var = selectUnassignedVariable(csp);
	for (Position value: orderDomainValues(var, assignment, csp))
	{
		std::vector<Position>lastPosition;
		std::vector<std::vector<Position>>lastDomains;
		csp.record(lastPosition,lastDomains);
		if (csp.consistent(value,assignment))
		{
			var->assign(value);
			assignment.push_back(var);
			std::vector<Queen*>inference = makeInference(csp, var, value);
			if (inference != FAILURE)
			{
				for (Queen* current : inference)
				{
					assignment.push_back(current);
				}
				result = backtrack(assignment, csp);
				if (result != FAILURE)
				{
					return result;
				}
			}
		}
		csp.recover(lastPosition,lastDomains);
		refresh(assignment);
	}
	return FAILURE;
}

std::vector<Queen*> search::minConflict(Csp& csp, int maxSteps)
{
	/*
	 * TODO
	 * Algorithm (Reference: Figure 6.8):
	 function MIN-CONFLICTS(csp,max steps) returns a solution or failure
		inputs: csp, a constraint satisfaction problem
				max steps, the number of steps allowed before giving up
		current<-an initial complete assignment for csp	
		for i = 1 to max steps do
			if current is a solution for csp then # use isSolution
				print how many steps used here
				return current 
			var <- a randomly chosen conflicted variable from csp.VARIABLES # use chooseConflictVariable
			value <- the value v for var that minimizes CONFLICTS(var, v, current , csp) # use getMinConflictValue
			set var =value in current	# use var->position = value
		return failure
	 */
	std::vector<Queen*>FAILURE({ NULL });
	std::vector<Queen*>current=csp.variables;
	for (int i = 0; i < maxSteps; i++)
	{
		if (isSolution(csp, current))
		{
			std::cout << i + 1 << "steps used here" << std::endl;
			return current;
		}
		Queen* var=chooseConflictVariable(csp);
		Position value=getMinConflictValue(csp, var);
		var->position = value;
	}

	return FAILURE;
}

std::vector<Queen*> search::minConflictWrapper(Csp& csp)
{
	csp.randomAssign();
	return minConflict(csp, 200);
}

int search::getConflicts(Csp& csp, Position& position)
{
	/*
	* TODO
	* 得到一个position在当前棋盘上的冲突数量
	* 注意：与position在同一列的queen的冲突不应该计算
	* 样例：
	*	0 1 0 0
		1 0 0 0
		0 0 1 0
		0 0 0 1
	* Position{0, 0}的冲突数应该为3，因为它与{0, 1},{2, 2},{3, 3}冲突
	* Position{1, 0}的冲突数量应该为1，因为它与{0, 1}冲突
	*/
	int conflictNumber = 0;
	for (Queen* current : csp.variables)
	{
		if (current->position.col == position.col && current->position.row == position.row)continue;
		else if (current->position.col != position.col && current->position.row == position.row)
		{
			++conflictNumber;
		}
		else if (abs(current->position.col - position.col) == abs(current->position.row - position.row))
		{
			++conflictNumber;
		}
	}
	return conflictNumber;
}

Queen* search::chooseConflictVariable(Csp& csp)
{
	/*
	* TODO
	* 返回一个目前赋值的冲突数大于0的variable
	* 注意：冲突数大于0的variable可能有多个，需要随机选择
	* 样例：
	*	0 1 0 0
		1 0 0 0
		0 0 1 0
		0 0 0 1
	* Queen1-4的冲突数都大于0，随机选择一个作为该函数的返回结果
	*/
	std::vector<Queen*> conflictQueens;
	for (Queen* current1 : csp.variables)
	{
		for (Queen* current2 : csp.variables)
		{
			if (current1 != current2)
			{
				if (current1->position.col != current2->position.col&& current1->position.row == current2->position.row)
				{
					conflictQueens.push_back(current1);
				}
				else if (abs(current1->position.col - current2->position.col) == abs(current1->position.row - current2->position.row))
				{
					conflictQueens.push_back(current1);
				}
			}
		}
	}
	return conflictQueens[rand() % conflictQueens.size()];
}

Position search::getMinConflictValue(Csp& csp, Queen* var)
{
	/*
	* TODO
	* 返回var的domian中，可以使冲突数最小的值
	* 注意：使冲突数最小的值可能有多个，需要随机选择，如果不随机选择问题可能会陷入局部稳定点并且该稳定点不是解
	* 样例：
	*	1 1 0 0
		0 0 0 0
		0 0 1 0
		0 0 0 1
	* Queen1所在的位置的冲突数为3，它的domain为{[0-3], 0}。{1, 0},{2, 0},{3, 0}的冲突数都为1。
	* 需要从中随机选取一个作为返回值。
	*/
	int conflictNmbers[8];
	std::vector<int>position{0};
	for (int i = 0; i < 8; i++)
	{
		conflictNmbers[i] = getConflicts(csp, var->domain[i]);
		if (conflictNmbers[i] < conflictNmbers[position[0]])
		{
			position.clear();
			position.push_back(i);
		}
		else if (conflictNmbers[i] == conflictNmbers[position[0]]&&i!=position[0])
		{
			position.push_back(i);
		}
	}
	return var->domain[position[rand() % position.size()]];
}

void search::printSolution(std::vector<Queen*>& solution)
{
	if (search::failed(solution))
	{
		std::cout << "No Valid Solution!" << std::endl;
		return;
	}

	int size = (int)solution.size();
	std::vector<std::vector<bool>> grid(size, std::vector<bool>(size, false));

	for (Queen* queen : solution)
	{
		grid[queen->position.row][queen->position.col] = true;
	}

	for (int row = 0; row < size; row++)
	{
		for (int col = 0; col < size; col++)
		{
			std::cout << grid[row][col] << " ";
		}
		std::cout << "\n";
	}
}
