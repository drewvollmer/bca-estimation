% get_permutations.m
% A function returning a vector with the permutations of all input matrices
% Drew Vollmer 02-02-2017

function permutation = get_permutations(varargin)

    % If there is only one argument, return that argument (recursive base case)
    if nargin == 1;
        permutation = varargin{1};

    elseif nargin >= 2;

        base_matrix = varargin{1};
        vec2 = varargin{2};

        % Calculate rows in base_matrix so it doesn't need to be constantly
        % recalculated
        base_matrix_rows = size(base_matrix, 1);

        % Create a permutation vector of the appropriate length, plus a column
        % of zeros
        permutation = [repmat(base_matrix, length(vec2), 1) zeros(base_matrix_rows .* length(vec2), 1)];
        % Get index of last column, so we know what to replace
        last_col = size(permutation, 2);

        for i = 1:size(vec2, 1);
            permutation( ((i - 1).*base_matrix_rows + 1):(i.*base_matrix_rows), last_col) = vec2(i);
        end;

        % Now update recursively!
        if nargin >= 3;
            permutation = get_permutations(permutation, varargin{3:end});
        end;

    end;

end
