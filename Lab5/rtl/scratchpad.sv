module scratchpad #(
    parameter int unsigned ARRAY = 8, MAX_TILE = 64, MAX_K = 128
) (
    input logic clk,
    input logic input_write, input_matrix, input_buffer,
    input logic [31:0] input_row, input_column, input_lanes,
    input logic [63:0] input_data,
    input logic compute_active, compute_buffer,
    input logic [31:0] step, sub_m, sub_n, tile_m, tile_n, tile_k,
    output logic signed [15:0] left_a [ARRAY], top_b [ARRAY],
    output logic [ARRAY-1:0] left_valid, top_valid,
    input logic c_write, c_zero,
    input logic [31:0] c_row, c_column, c_lanes,
    input logic signed [31:0] c_values [ARRAY],
    input logic [31:0] read_row, read_column,
    output logic [63:0] read_data
);
    localparam int unsigned A_WORDS = (MAX_K + 3) / 4;
    localparam int unsigned COLUMN_GROUPS = MAX_TILE / ARRAY;
    // A bank serves one matrix row and stores four int16 elements per word.
    logic [63:0] a_memory [2][ARRAY][MAX_TILE/ARRAY*A_WORDS];
    // B and C are banked by column; one compute read/write per bank per cycle.
    logic signed [15:0] b_memory [2][ARRAY][MAX_K*COLUMN_GROUPS];
    logic signed [31:0] c_memory [ARRAY][MAX_TILE*COLUMN_GROUPS];

    always_ff @(posedge clk) begin
        if (input_write) begin
            if (!input_matrix)
                a_memory[input_buffer][input_row % ARRAY][input_row / ARRAY * A_WORDS + input_column / 4] <= input_data;
            else
                for (int lane = 0; lane < 4; ++lane)
                    if (32'(lane) < input_lanes)
                        b_memory[input_buffer][(input_column + 32'(lane)) % ARRAY]
                            [input_row * COLUMN_GROUPS + (input_column + 32'(lane)) / ARRAY]
                            <= $signed(input_data[lane*16 +: 16]);
        end
        if (c_write)
            for (int lane = 0; lane < ARRAY; ++lane)
                if (32'(lane) < c_lanes) begin
                    if (c_zero)
                        c_memory[(c_column+32'(lane))%ARRAY][c_row*COLUMN_GROUPS+(c_column+32'(lane))/ARRAY] <= 0;
                    else
                        c_memory[(c_column+32'(lane))%ARRAY][c_row*COLUMN_GROUPS+(c_column+32'(lane))/ARRAY]
                            <= c_memory[(c_column+32'(lane))%ARRAY][c_row*COLUMN_GROUPS+(c_column+32'(lane))/ARRAY] + c_values[lane];
                end
    end

    always_comb begin
        left_valid = '0; top_valid = '0;
        for (int lane = 0; lane < ARRAY; ++lane) begin
            left_a[lane] = 0; top_b[lane] = 0;
            // Index skew at the array edges aligns equal-k operands in every PE.
            if (compute_active && sub_m+32'(lane)<tile_m && step>=32'(lane) && step-32'(lane)<tile_k) begin
                left_valid[lane] = 1'b1;
                left_a[lane] = $signed(a_memory[compute_buffer][lane]
                    [sub_m/ARRAY*A_WORDS+(step-32'(lane))/4][16*((step-32'(lane))%4) +: 16]);
            end
            if (compute_active && sub_n+32'(lane)<tile_n && step>=32'(lane) && step-32'(lane)<tile_k) begin
                top_valid[lane] = 1'b1;
                top_b[lane] = b_memory[compute_buffer][lane][(step-32'(lane))*COLUMN_GROUPS+sub_n/ARRAY];
            end
        end
        read_data = 0;
        read_data[31:0] = c_memory[read_column%ARRAY][read_row*COLUMN_GROUPS+read_column/ARRAY];
        if (read_column+1<tile_n)
            read_data[63:32] = c_memory[(read_column+1)%ARRAY][read_row*COLUMN_GROUPS+(read_column+1)/ARRAY];
    end
endmodule
