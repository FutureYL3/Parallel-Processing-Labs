module pe (
    input logic clk, rst_n, clear, enable,
    input logic signed [15:0] a_in, b_in,
    input logic a_valid, b_valid,
    output logic signed [15:0] a_out, b_out,
    output logic a_valid_out, b_valid_out,
    output logic signed [31:0] sum
);
    logic signed [31:0] product;
    assign product = a_in * b_in;
    always_ff @(posedge clk) begin
        if (!rst_n || clear) begin
            a_out <= '0; b_out <= '0;
            a_valid_out <= 1'b0; b_valid_out <= 1'b0; sum <= '0;
        end else if (enable) begin
            a_out <= a_in; b_out <= b_in;
            a_valid_out <= a_valid; b_valid_out <= b_valid;
            if (a_valid && b_valid) sum <= sum + product;
        end
    end
endmodule
