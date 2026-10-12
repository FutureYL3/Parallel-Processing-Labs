module systolic_array #(parameter int unsigned ARRAY = 8) (
    input logic clk, rst_n, clear, enable,
    input logic signed [15:0] left_a [ARRAY], top_b [ARRAY],
    input logic [ARRAY-1:0] left_valid, top_valid,
    output logic signed [31:0] sums [ARRAY][ARRAY],
    output logic [31:0] macs_this_cycle
);
    logic signed [15:0] a_path [ARRAY][ARRAY], b_path [ARRAY][ARRAY];
    logic a_out_valid [ARRAY][ARRAY], b_out_valid [ARRAY][ARRAY];
    logic a_in_valid [ARRAY][ARRAY], b_in_valid [ARRAY][ARRAY];
    for (genvar r = 0; r < ARRAY; ++r) begin : rows
        for (genvar c = 0; c < ARRAY; ++c) begin : columns
            logic signed [15:0] a_input, b_input;
            if (c == 0) begin : left_edge
                assign a_input = left_a[r]; assign a_in_valid[r][c] = left_valid[r];
            end else begin : horizontal_link
                assign a_input = a_path[r][c-1]; assign a_in_valid[r][c] = a_out_valid[r][c-1];
            end
            if (r == 0) begin : top_edge
                assign b_input = top_b[c]; assign b_in_valid[r][c] = top_valid[c];
            end else begin : vertical_link
                assign b_input = b_path[r-1][c]; assign b_in_valid[r][c] = b_out_valid[r-1][c];
            end
            pe cell_pe (
                .clk, .rst_n, .clear, .enable, .a_in(a_input), .b_in(b_input),
                .a_valid(a_in_valid[r][c]), .b_valid(b_in_valid[r][c]),
                .a_out(a_path[r][c]), .b_out(b_path[r][c]),
                .a_valid_out(a_out_valid[r][c]), .b_valid_out(b_out_valid[r][c]), .sum(sums[r][c])
            );
        end
    end
    always_comb begin
        macs_this_cycle = 0;
        for (int r = 0; r < ARRAY; ++r)
            for (int c = 0; c < ARRAY; ++c)
                if (a_in_valid[r][c] && b_in_valid[r][c]) macs_this_cycle += 1;
    end
endmodule
